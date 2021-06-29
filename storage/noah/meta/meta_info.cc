// Author: Zhenwei Zhu losted.leo@gmail.com

#include "meta/meta_info.h"

#include <sstream>
#include <gflags/gflags.h>

#include "butil/time.h"
#include "butil/logging.h"
#include "butil/strings/string_number_conversions.h"
#include "butil/strings/string_split.h"
#include "butil/hash.h"

DEFINE_int32(node_timeout_ms, 5000, "NodeServer timeout in ms");

namespace noah {
namespace meta {

std::string Node2String(const Node& node) {
  return node.ip() + ":" + butil::IntToString(node.port());
}

void String2Node(const std::string& ip_and_port, Node* node) {
  std::vector<std::string> ip_info;
  butil::SplitString(ip_and_port, ':', &ip_info);
  if (ip_info.size() != 2) {
    return;
  }
  node->set_ip(ip_info[0]);
  int32_t port = 0;
  butil::StringToInt(ip_info[1], &port);
  node->set_port(port);
}

void UpdateNodeTable(const Node& node, const std::string& table_name,
                     NodeTableMap* node_table) {
  std::string node_str = Node2String(node);
  auto iter = node_table->find(node_str);
  if (iter != node_table->end()) {
    iter->second.insert(table_name);
  } else {
    std::set<std::string> tables;
    tables.insert(table_name);
    node_table->insert(std::make_pair(node_str, tables));
  }
}

void SetNodeTable(const Table& table, NodeTableMap* node_table) {
  const RepeatedPartition& partitions = table.partitions();
  for (const auto& value : partitions) {
    UpdateNodeTable(value.master(), table.name(), node_table);
    const google::protobuf::RepeatedPtrField<Node>& nodes = value.slaves();
    for (const auto& node : nodes) {
      UpdateNodeTable(node, table.name(), node_table);
    }
  }
}

void ClearNodeTable(const Node& node, const std::string& table_name,
                     NodeTableMap* node_table) {
  std::string node_str = Node2String(node);
  auto iter = node_table->find(node_str);
  if (iter != node_table->end()) {
    iter->second.erase(table_name);
  }
}

void DropNodeTable(const Table& table, NodeTableMap* node_table) {
  const RepeatedPartition& partitions = table.partitions();
  for (const auto& value : partitions) {
    ClearNodeTable(value.master(), table.name(), node_table);
    const google::protobuf::RepeatedPtrField<Node>& nodes = value.slaves();
    for (const auto& node : nodes) {
      ClearNodeTable(node, table.name(), node_table);
    }
  }
}

std::string VersionInfo::DebugDump() {
  std::stringstream ss;
  ss << "epoch: " << epoch << " version: " << version;
  return ss.str();
}

bool MetaInfoSnapshot::CheckExpiredNodes(std::vector<std::string>* expired_nodes) {
  bool node_change = false;
  for (auto& node : nodes) {
    if (node.second.last_alive_time > 0
        && (butil::gettimeofday_us() - node.second.last_alive_time
            > (uint64_t)FLAGS_node_timeout_ms * 1000L)) {
      node_change = true;
      node.second.last_alive_time = 0;
      if (expired_nodes) {
        expired_nodes->push_back(node.first);
      }
    }
  }
  return node_change;
}

void MetaInfoSnapshot::GetAliveNodes(std::set<Node, node_less>* alive_nodes) const {
  alive_nodes->clear();
  Node node;
  for (const auto& value : nodes) {
    if (value.second.last_alive_time > 0) {
      String2Node(value.first, &node);
      alive_nodes->insert(node);
    }
  }
}

MetaInfo::MetaInfo() {
  version_.epoch = -1;
  version_.version = -1;
}

MetaInfo::~MetaInfo() {
}

VersionInfo MetaInfo::version() {
  butil::WriterAutoLock lock_r(&meta_rw_);
  return version_;
}

bool MetaInfo::Init() {
  if (!Initialized()) {
    butil::WriterAutoLock lock_r(&meta_rw_);
    version_.epoch = 0;
    version_.version = 0;
  }
  return true;
}

void MetaInfo::RefreshNodes() {
  uint64_t current_time = butil::gettimeofday_us();
  butil::WriterAutoLock lock_w(&meta_rw_);
  for (auto& node : nodes_) {
    node.second.last_alive_time == 0 ? 0 : (node.second.last_alive_time = current_time);
    LOG(INFO) << "RefreshNode " << node.first << " node.second.last_alive_time "
      << node.second.last_alive_time;
  }
}

VersionInfo MetaInfo::UpdateNode(const Node& node) {
  std::string ip_str = Node2String(node);

  butil::WriterAutoLock lock_w(&meta_rw_);
  auto iter = nodes_.find(ip_str);
  if (iter != nodes_.end()) {
    // if it is fail back
    if (iter->second.last_alive_time == 0) {
      LOG(INFO) << "node " << ip_str << " faliback";
      iter->second.last_alive_time = butil::gettimeofday_us();
      UpdatePartNodeMS(node);
      version_.epoch++;
    }
    iter->second.last_alive_time = butil::gettimeofday_us();
  } else {
    nodes_[ip_str] = NodeInfo(butil::gettimeofday_us());
    version_.epoch++;
  }
  return version_;
}

VersionInfo MetaInfo::CreateTable(const VersionInfo& info, const Table& table) {
  butil::WriterAutoLock lock_w(&meta_rw_);
  if (info != version_) {
    return VersionInfo(-1, 0);
  }
  auto iter = table_info_.find(table.name());
  if (iter != table_info_.end()) {
    return VersionInfo(0, 0);
  }
  SetNodeTable(table, &node_table_);
  table_info_[table.name()] = table;
  table_info_[table.name()].set_version(++version_.version);

  return version_;
}

VersionInfo MetaInfo::DownNodes(int64_t epoch, const std::vector<Node>& nodes,
                                MigrateTaskArray* tasks) {
  butil::WriterAutoLock lock_w(&meta_rw_);
  if (epoch != version_.epoch) {
    return VersionInfo();
  }
  std::string node_str;
  bool node_change = false;
  for (const auto& node : nodes) {
    node_str = Node2String(node);
    auto iter = nodes_.find(node_str);
    if (iter != nodes_.end()) {
      node_change = true;
      iter->second.last_alive_time = 0;
    }
  }
  if (node_change) {
    version_.epoch++;
    GenerateMigrateTasks(nodes, tasks);
    LOG(INFO) << "DOWNNODE cur node size  " << nodes_.size();
  }
  return version_;
}

VersionInfo MetaInfo::DropTable(const VersionInfo& info, const StringVector& tables) {
  butil::WriterAutoLock lock_w(&meta_rw_);
  if (info != version_) {
    return VersionInfo();
  }
  bool changed = false;
  for (const auto& table : tables) {
    auto iter = table_info_.find(table);
    if (iter != table_info_.end()) {
      DropNodeTable(iter->second, &node_table_);
      table_info_.erase(iter);
      changed = true;
    }
  }
  if (changed) {
    ++version_.version;
  }

  return version_;
}

VersionInfo MetaInfo::CommitMigrate(const VersionInfo& info, const Processor& processor) {
  butil::WriterAutoLock lock_w(&meta_rw_);
  if (info != version_) {
    return VersionInfo();
  }
  auto iter = table_info_.find(processor.task.table_name);
  if (iter == table_info_.end()) {
    LOG(ERROR) << "Table " << processor.task.table_name << " not exist";
    return VersionInfo();
  }
  auto partition = iter->second.mutable_partitions(processor.task.partition_id);
  if (node_equal()(partition->master(), processor.dst)) {
    LOG(WARNING) << "Dst is master already " << Node2String(processor.dst);
    return version_;
  }
  for (const auto& slave : partition->slaves()) {
    if (node_equal()(slave, processor.dst)) {
      LOG(WARNING) << "Dst is slave already " << Node2String(processor.dst);
      return version_;
    }
  }
  if (Role::MASTER == processor.task.fault_node.role) {
    *partition->add_slaves() = partition->master();
    *partition->mutable_master() = processor.dst;
  } else {
    *partition->add_slaves() = processor.dst;
  }
  UpdateNodeTable(processor.dst, processor.task.table_name, &node_table_);
  VLOG(10) << "Table info " << iter->second.DebugString();
  if (1 + partition->slaves_size() == iter->second.duplicate_num()) {
    partition->set_status(PStatus::ACTIVE);
  }

  iter->second.set_version(++version_.version);
  return version_;
}

bool MetaInfo::ExistTable(const std::string& table_name) {
  butil::ReaderAutoLock lock_r(&meta_rw_);
  if (table_info_.find(table_name) != table_info_.end()) {
    return true;
  }
  return false;
}

bool MetaInfo::Changed(const VersionInfo& info) {
  butil::ReaderAutoLock lock_r(&meta_rw_);
  if (info.epoch < version_.epoch || info.version < version_.version) {
    return true;
  }
  return false;
}

bool MetaInfo::LoadFromSnapshot(MetaInfoSnapshot* snapshot) {
  butil::WriterAutoLock lock_w(&meta_rw_);
  if (snapshot->version.epoch <= version_.epoch
      && snapshot->version.version <= version_.version) {
    return false;
  }

  version_ = snapshot->version;
  table_info_.swap(snapshot->table_info);
  node_table_.swap(snapshot->node_table);
  nodes_.swap(snapshot->nodes);

  return true;
}

void MetaInfo::GetSnapshot(MetaInfoSnapshot* snapshot) {
  butil::ReaderAutoLock lock_r(&meta_rw_);
  snapshot->version = version_;
  snapshot->table_info = table_info_;
  snapshot->node_table = node_table_;
  snapshot->nodes = nodes_;
}

bool MetaInfo::Initialized() {
  butil::ReaderAutoLock lock_r(&meta_rw_);
  return version_.epoch > -1;
}

void MetaInfo::GenerateMigrateTasks(const std::vector<Node>& nodes,
                                    MigrateTaskArray* tasks) {
  for (const auto& node : nodes) {
    MigrateFromNode(node, tasks);
  }
}

void MetaInfo::MigrateFromNode(const Node& node, MigrateTaskArray* tasks) {
  std::string node_str = Node2String(node);
  std::set<std::string> fault_tables = node_table_[node_str];
  node_table_.erase(node_str);
  for (const auto& table : fault_tables) {
    auto iter = table_info_.find(table);
    if (iter == table_info_.end()) {
      LOG(ERROR) << "Table " << table << " should exist";
      continue;
    }
    MigrateFromTable(node, &iter->second, tasks);
  }
}

void MetaInfo::MigrateFromTable(const Node& node, Table* table,
                                MigrateTaskArray* tasks) {
  size_t size = table->partitions_size();
  MigrateTask task;
  // down node from table_part  relase master and slave;
  for (size_t i = 0; i < size; ++i) {
    bool changed = false;
    auto partition = table->mutable_partitions(i);
    if (node_equal()(node, partition->master())) {
      partition->release_master();
      changed = true;
    } else {
      auto slaves = partition->slaves();
      partition->clear_slaves();
      for (const auto& slave : slaves) {
        if (node_equal()(node, slave)) {
             changed = true;
        } else {
          *partition->add_slaves() = slave;
        }
      }
    }

    // If partition changed set partition status
    if (changed) {
      table->set_version(version_.version);
      if (partition->has_master() || partition->slaves_size()) {
        // Set first salve to master
        bool isFind = false;
        if (!partition->has_master()) {
          auto slaves = partition->slaves();
          partition->clear_slaves();
          for (const auto& slave : slaves) {
            std::string node_str = Node2String(slave);
            auto iter = nodes_.find(node_str);
            if (iter != nodes_.end() && !isFind) {
              *partition->mutable_master() = slave;
              isFind = true;
              LOG(INFO) << "part change master " << table->name() << " partid " << partition->id()
                << " master " << Node2String(partition->master());
              continue;
            }
            *partition->add_slaves() = slave;
          }
        }

         partition->set_status(PStatus::SLOWDOWN);
        if (!partition->has_master()) {
          LOG(INFO) << "part change master " << table->name() << " partid " << partition->id()
            << " no master ";
          partition->set_status(PStatus::STUCK);
        }
      } else {
        // this part no master and no slaves .so set it is STUCK;
        partition->set_status(PStatus::STUCK);
        LOG(INFO) << "part change master " << table->name() << " partid " << partition->id()
          << " no master no slave  ";
      }
    }
  }
}

bool MetaInfo::UpdatePartNodeMS(const Node& node) {
  noah::common::NodeArray nodes;
  for (auto& node_info : nodes_) {
    Node node_tmp;
    String2Node(node_info.first, &node_tmp);
    noah::common::NodeStatus node_status = \
      noah::common::NodeStatus(node_tmp, node_info.second.last_alive_time == 0 ? \
      noah::meta::NodeState::DOWN : noah::meta::NodeState::UP);
    nodes.push_back(node_status);
  }

  bool change = false;
  for (auto &value : table_info_) {
    std::string table_name = value.first;
    uint32_t table_hash = butil::Hash(table_name);
    int32_t start_index = table_hash % nodes_.size();
    Table * table = &(value.second);
    if (NULL == table) { continue; }
    size_t part_size = table->partitions_size();
    size_t dup_size = table->duplicate_num();

    for (size_t i = 0; i < part_size; ++i) {
      auto partition = table->mutable_partitions(i);

      if (partition->status() == PStatus::ACTIVE) continue;
      for (size_t j = 0; j < dup_size; ++j) {
        int node_id = (start_index + i + j) % nodes_.size();
        Node  part_node;
        String2Node(nodes[node_id].node().ToString(), &part_node);
        if (node_equal()(node, part_node)) {
          LOG(INFO) << "find node part " << Node2String(node) << " table_name " << table_name
            << " part " << i << " j = " << j;
          if (j == 0) {
            if (!partition->has_master()) {
              *partition->mutable_master() = node;
            } else {
              Node node_master = partition->master();
              if (!node_equal()(node, node_master)) {
                *partition->mutable_master() = node;
                *partition->add_slaves() = node_master;
                change = true;
              }
            }
          } else {
            // add slave
            *partition->add_slaves() = node;
            change = true;
          }
        }
        UpdateNodeTable(node, table_name, &node_table_);
      }
      // change part ACTIVE
      size_t slave_size = partition->slaves_size();
      if (partition->has_master() && (1 + slave_size) == dup_size) {
        partition->set_status(PStatus::ACTIVE);
        LOG(INFO) << "change part " << table_name << " " << i << " active!";
      } else if (partition->has_master() || slave_size > 0) {
        partition->set_status(PStatus::SLOWDOWN);
        LOG(INFO) << "change part " << table_name << " " << i << " slowdown";
      } else if (!partition->has_master() &&  slave_size == 0) {
        partition->set_status(PStatus::STUCK);
        LOG(INFO) << "change part " << table_name << " " << i << " stuck";
      }

      if (change) {
        table->set_version(version_.version);
      }
    }
  }

  return true;
}
}  // namespace meta
}  // namespace noah
