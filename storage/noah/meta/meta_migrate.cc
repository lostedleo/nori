// Author: Zhenwei Zhu losted.leo@gmail.com

#include "meta/meta_migrate.h"

#include <sstream>

#include "butil/logging.h"

#include "meta/meta_info.h"

namespace noah {
namespace meta {
Processor::Processor(const MigrateUnit& unit) {
  task.table_name = unit.name();
  task.partition_id = unit.id();
  task.fault_node.node = unit.fault();
  task.fault_node.role = unit.role();
  src = unit.src();
  dst = unit.dst();
}

std::string Processor::ToString() {
  std::stringstream ss;
  ss << "Processor info " << DebugString(task) << " from " << Node2String(src)
     << " to " << Node2String(dst);
  return ss.str();
}

std::string DebugString(const MigrateTask& task) {
  std::stringstream ss;
  ss << "table_name: " << task.table_name << " id: " << task.partition_id
     << " fault_node: " << Node2String(task.fault_node.node) << " role: "
     << (task.fault_node.role > 0 ? "salve" : "master");
  return ss.str();
}

std::string DebugString(const MigrateTaskArray& tasks) {
  std::stringstream ss;
  ss << "MigrateTaskArray is:" << std::endl;
  for (const auto& task : tasks) {
    ss << DebugString(task) << std::endl;
  }
  return ss.str();
}

std::string Task2String(const MigrateTask& task) {
  std::stringstream ss;
  ss << task.table_name << "_" << task.partition_id << "_"
     << Node2String(task.fault_node.node) << "_" << int32_t(task.fault_node.role);
  return ss.str();
}

std::string ProcessorString(const std::string table_name, int32_t id,
                            const Node& dst) {
  std::stringstream ss;
  ss << table_name << "_" << id << "_" << Node2String(dst);
  return ss.str();
}

MetaMigrate::MetaMigrate() {
}

MetaMigrate::~MetaMigrate() {
}

bool MetaMigrate::HasMigrateTask() {
  butil::AutoLock auto_lock(lock_);
  return migrate_tasks_.size() ? true : false;
}

void MetaMigrate::AddMigrateTasks(const MigrateTaskArray& tasks) {
  butil::AutoLock auto_lock(lock_);
  for (const auto& task : tasks) {
    migrate_tasks_[Task2String(task)] = task;
  }
}

bool MetaMigrate::GetMigrateTask(const Node& node, const MetaInfoSnapshot* snapshot,
                                 Processor* processor) {
  butil::AutoLock auto_lock(lock_);
  if (migrate_tasks_.empty()) {
    return false;
  }
  std::set<Node, node_less> alive_nodes;
  snapshot->GetAliveNodes(&alive_nodes);
  for (const auto& task : migrate_tasks_) {
    auto iter = snapshot->table_info.find(task.second.table_name);
    if (iter == snapshot->table_info.end()) {
      // Table not exist delete task
      migrate_tasks_.erase(task.first);
      continue;
    }
    auto partition = iter->second.partitions(task.second.partition_id);
    if (!partition.has_master()) {
      LOG(WARNING) << "Table " << task.second.table_name << " id "
                   << task.second.partition_id << " is stucked";
      migrate_tasks_.erase(task.first);
      continue;
    }
    if (!node_equal()(partition.master(), node)) {
      continue;
    }
    Node candidate;
    if (!FindCandidateNode(iter->second, partition, alive_nodes,
                           task.second.fault_node.node, &candidate)) {
      continue;
    }
    processor->task = task.second;
    processor->src = partition.master();
    processor->dst = candidate;

    std::string processor_str = ProcessorString(task.second.table_name,
                                                task.second.partition_id,
                                                candidate);
    auto t_iter = migrating_info_.find(task.second.table_name);
    if (t_iter == migrating_info_.end()) {
      NodeIntMap node_info;
      node_info.insert(std::make_pair(candidate, 1));
      migrating_info_.insert(std::make_pair(task.second.table_name, node_info));
    } else {
      auto n_iter = t_iter->second.find(candidate);
      if (n_iter != t_iter->second.end()) {
        n_iter->second++;
      } else {
        t_iter->second.insert(std::make_pair(candidate, 1));
      }
    }
    processor_info_[processor_str] = *processor;
    migrate_tasks_.erase(task.first);

    return true;
  }
  return false;
}

bool MetaMigrate::CommitMigrate(const Processor& processor) {
  butil::AutoLock auto_lock(lock_);
  std::string processor_str = ProcessorString(processor.task.table_name,
                                              processor.task.partition_id,
                                              processor.dst);
  processor_info_.erase(processor_str);
  migrate_tasks_.erase(Task2String(processor.task));

  auto t_iter = migrating_info_.find(processor.task.table_name);
  if (t_iter != migrating_info_.end()) {
    auto n_iter = t_iter->second.find(processor.dst);
    if (n_iter != t_iter->second.end()) {
      n_iter->second--;
      if (0 == n_iter->second) {
        t_iter->second.erase(n_iter);
      }
    }
  }

  return true;
}

bool MetaMigrate::FindCandidateNode(const Table& table, const Partition& partition,
                                    const std::set<Node, node_less>& alive_nodes,
                                    const Node& fault_node, Node* dst) {
  std::set<Node, node_less> candidate(alive_nodes);
  const Node& node = partition.master();
  auto iter = candidate.find(node);
  if (iter != candidate.end()) {
    candidate.erase(iter);
  }
  for (const auto& slave : partition.slaves()) {
    iter = candidate.find(slave);
    if (iter != candidate.end()) {
      candidate.erase(iter);
    }
  }

  std::map<Node, int, node_less> node_partitions;
  iter = candidate.begin();
  std::string processor_str;
  for (; iter != candidate.end();) {
    processor_str = ProcessorString(table.name(), partition.id(), *iter);
    if (processor_info_.find(processor_str) != processor_info_.end()) {
      iter = candidate.erase(iter);
    } else {
      int number = GetMigratingNumber(table.name(), *iter);
      node_partitions.insert(std::make_pair(*iter, number));
      ++iter;
    }
  }

  if (candidate.empty()) {
    return false;
  } else if (1 == candidate.size()) {
    *dst = *candidate.begin();
    return true;
  }

  for (const auto& value : table.partitions()) {
    if (candidate.find(value.master()) != candidate.end()) {
      node_partitions[value.master()]++;
    }
    for (const auto& slave : value.slaves()) {
      if (candidate.find(slave) != candidate.end()) {
        node_partitions[slave]++;
      }
    }
  }
  auto miter = node_partitions.begin();
  *dst = miter->first;
  int32_t count = miter->second;
  while (++miter != node_partitions.end()) {
    if (count > miter->second) {
      *dst = miter->first;
      count = miter->second;
    } else if (count == miter->second) {
      // If fault_node in condidates then first use it
      if (node_equal()(fault_node, miter->first)) {
        *dst = miter->first;
      }
    }
  }
  return true;
}

int32_t MetaMigrate::GetMigratingNumber(const std::string& table_name,
                                        const Node& node) {
  int32_t number = 0;
  auto iter = migrating_info_.find(table_name);
  if (iter != migrating_info_.end()) {
    auto n_iter = iter->second.find(node);
    if (n_iter != iter->second.end()) {
      number = n_iter->second;
    }
  }

  return number;
}

}  // namespace meta
}  // namespace noah
