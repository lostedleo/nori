// Author: Zhenwei Zhu losted.leo@gmail.com

#include "common/entity.h"

#include <sstream>

#include "butil/strings/string_number_conversions.h"
#include "butil/strings/string_split.h"
#include "butil/hash.h"

#include "meta/meta.pb.h"

namespace noah {
namespace common {
Node::Node(const std::string& ip_and_port) {
  std::vector<std::string> ip_info;
  butil::SplitString(ip_and_port, ':', &ip_info);
  if (ip_info.size() != 2) {
    port = 0;
    return;
  }
  ip = ip_info[0];
  butil::StringToInt(ip_info[1], &port);
}

Node::Node(const noah::meta::Node& node) {
  ip = node.ip();
  port = node.port();
}

std::string Node::ToString() const {
  return ip + ":" + butil::IntToString(port);
}

Partition::Partition() : id_(-1), state_(State::kUnknow) {
}

Partition::Partition(const noah::meta::Partition& partition)
  : id_(partition.id()),
    master_(partition.master()) {
  for (int i = 0; i < partition.slaves_size(); i++) {
    slaves_.push_back(Node(partition.slaves(i)));
  }
  id_ = partition.id();

  switch (partition.status()) {
    case noah::meta::PStatus::ACTIVE:
      state_ = kActive;
      break;
    case noah::meta::PStatus::STUCK:
      state_ = kStuck;
      break;
    case noah::meta::PStatus::SLOWDOWN:
      state_ = kSlowDown;
      break;
    default:
      state_ = kUnknow;
      break;
  }
}

std::string Partition::DebugDump() const {
  std::stringstream ss;
  std::string state_str;
  switch (state_) {
    case kActive:
      state_str.assign("Active");
      break;
    case kStuck:
      state_str.assign("Stuck");
      break;
    case kSlowDown:
      state_str.assign("SlowDown");
      break;
    default:
      state_str.assign("Unknow");
      break;
  }
  ss << " -" << id_ << ", status: " << state_str << ", master: " << master_.ip
     << ":" << master_.port;
  for (auto& s : slaves_) {
    ss << " slave: " << s.ip << ":" <<  s.port;
  }
  ss << std::endl;
  return ss.str();
}

void Partition::SetMaster(const Node& new_master) {
  master_.ip = new_master.ip;
  master_.port = new_master.port;
}

Table::Table()
  : partition_num_(0), version_(0), capacity_(0) {
}

Table::Table(const noah::meta::Table& table) {
  table_name_ = table.name();
  version_ = table.version();
  partition_num_ = table.partitions_size();
  capacity_ = table.capacity();
  noah::meta::Partition partition;
  for (int i = 0; i < table.partitions_size(); i++) {
    partition = table.partitions(i);
    partitions_.insert(std::make_pair(partition.id(),
           Partition(partition)));
  }
}

Table::~Table() {
}

const Partition* Table::GetPartition(const std::string& key) const {
  if (partitions_.empty()) {
    return NULL;
  }
  int par_num = butil::Hash(key) % partitions_.size();

  return GetPartitionById(par_num);
}

const Partition* Table::GetPartitionById(int par_num) const {
  if (partitions_.empty()) {
    return NULL;
  }
  auto iter = partitions_.find(par_num);
  if (iter != partitions_.end()) {
    return &(iter->second);
  }
  return NULL;
}

std::string Table::DebugDump(int partition_id) const {
  std::stringstream ss;
  ss << " -table name: " << table_name_ << " version: " << version_
     << " capacity: " << capacity_ << std::endl;
  ss << " -partition num: " << partition_num_ << std::endl;
  if (partition_id != -1) {
    auto iter = partitions_.find(partition_id);
    if (iter != partitions_.end()) {
      ss << iter->second.DebugDump();
    } else {
      ss << " -partition: "<< partition_id << ", not exist" << std::endl;
    }
    return ss.str();
  }

  // Dump all
  for (auto& par : partitions_) {
    ss << par.second.DebugDump();
  }
  return ss.str();
}

void Table::GetAllMasters(std::vector<Node>* nodes) const {
  for (auto& par : partitions_) {
    nodes->push_back(par.second.master());
  }
}

void Table::GetAllMasters(std::set<Node>* nodes) const {
  for (auto& par : partitions_) {
    nodes->insert(par.second.master());
  }
}

void Table::GetAllNodes(std::set<Node>* nodes) const {
  for (auto& par : partitions_) {
    nodes->insert(par.second.master());
    for (auto& s : par.second.slaves()) {
      nodes->insert(s);
    }
  }
}

NodeStatus::NodeStatus(const noah::meta::NodeStatus& node_status) {
  node_ = noah::common::Node(node_status.node());
  state_ = node_status.status();
}

NodeStatus::NodeStatus(const noah::meta::Node& node, const noah::meta::NodeState state) {
  node_ = noah::common::Node(node);
  state_ = state;
}

std::string NodeStatus::DebugDump() const {
  std::stringstream ss;
  ss << "node " <<  node_.ToString() << " "
    << ((state_ == noah::meta::NodeState::DOWN) ? "DOWN":"UP");
  return ss.str();
}

VersionInfo::VersionInfo(const noah::meta::Version& info) {
  epoch = info.epoch();
  version = info.version();
}

VersionInfo::VersionInfo(int64_t epoch, int64_t version) {
  this->epoch = epoch;
  this->version = version;
}

std::string VersionInfo::DebugDump() const {
  std::stringstream ss;
  ss << "epoch: " << epoch << " version: " << version;
  return ss.str();
}

}  // namespace common
}  // namespace noah
