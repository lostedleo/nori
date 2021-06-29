// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_COMMON_ENTITY_H_
#define NOAH_COMMON_ENTITY_H_

#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <ostream>
#include "meta/meta.pb.h"
typedef std::unordered_map<std::string, std::string> KeyValueMap;
typedef std::vector<std::string> StringVector;

namespace noah {
namespace meta {
class Version;
class Node;
class Partition;
class Table;
class NodeStatus;
}

namespace common {

enum Status {
  kOk = 0,
  kError = 1,
  kRedirect = 2,
  kExist = 3,
  kNotFound = 4,
  kShouldUpdate = 5,
  kNotSupport = 6,
  kUnknow = 255,
};

enum Role {
  kMaster = 0,
  kSlave = 1,
};

enum KeyType {
  kMeta = 0,
  kKey = 1,
};

struct Node {
  explicit Node(const std::string& ip_and_port);
  explicit Node(const noah::meta::Node& node);
  Node() : port(0) {}
  Node(std::string other_ip, int32_t other_port)
    : ip(other_ip), port(other_port) {
  }
  bool operator< (const Node& other) const {
    return (ip < other.ip || (ip == other.ip && port < other.port));
  }

  bool operator== (const Node& other) const {
    return (ip == other.ip && port == other.port);
  }

  bool operator!= (const Node& other) const {
    return (ip != other.ip || port != other.port);
  }

  std::string ToString() const;

  std::string ip;
  int32_t port;
};

struct NodeHash {
  bool operator()(const Node& node) const {
    return std::hash<std::string>()(node.ip) ^ std::hash<int>()(node.port);
  }
};

class Partition {
 public:
  enum State {
    kActive,
    kSlowDown,
    kStuck,
    kUnknow,
  };

  Partition();
  explicit Partition(const noah::meta::Partition& partition);
  std::string DebugDump() const;
  Node master() const { return master_; }
  int id() const { return id_; }
  std::vector<Node> slaves() const { return slaves_; }
  State state() const { return state_; }
  void SetMaster(const Node& new_master);

 private:
  int id_;
  State state_;
  Node master_;
  std::vector<Node> slaves_;
};

class Table {
 public:
  Table();
  explicit Table(const noah::meta::Table& table);
  virtual ~Table();

  std::string table_name() const { return table_name_; }
  int partition_num() const { return partition_num_; }
  int64_t version() const { return version_; }
  int64_t capacity() const { return capacity_; }

  std::map<int, Partition> partitions() { return partitions_; }

  const Partition* GetPartition(const std::string& key) const;
  const Partition* GetPartitionById(int id) const;
  void GetAllMasters(std::vector<Node>* nodes) const;
  void GetAllMasters(std::set<Node>* nodes) const;
  void GetAllNodes(std::set<Node>* nodes) const;
  std::string DebugDump(int partition_id = -1) const;

 private:
  int partition_num_;
  int64_t version_;
  std::string table_name_;
  std::map<int, Partition> partitions_;
  int64_t capacity_;
};


class NodeStatus{
 public:
  explicit NodeStatus(const noah::meta::NodeStatus& node_status);
  explicit NodeStatus(const noah::meta::Node& node, const noah::meta::NodeState state);
  std::string DebugDump() const;
  Node node() const {return node_;}
  noah::meta::NodeState state() const {return state_;}

 private:
  Node node_;
  noah::meta::NodeState state_;
};

typedef std::unordered_map<std::string, Table> TableMap;
typedef std::vector<NodeStatus> NodeArray;

struct VersionInfo {
  VersionInfo() : epoch(-1), version(-1) {
  }
  explicit VersionInfo(const noah::meta::Version& info);
  VersionInfo(int64_t epoch, int64_t version);
  std::string DebugDump() const;

  int64_t   epoch;
  int64_t   version;
};

struct MetaInfo {
  MetaInfo() {
  }

  VersionInfo version;
  TableMap    table_info;
  NodeArray   nodes;
};

struct SpaceInfo {
  int64_t used;
  int64_t remain;
};

}  // namespace common
}  // namespace noah
#endif
