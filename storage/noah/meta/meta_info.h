// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_META_INFO_H_
#define NOAH_META_INFO_H_

#include <atomic>
#include <string>

#include <unordered_map>
#include <set>

#include "butil/synchronization/rw_mutex.h"
#include "common/entity.h"
#include "meta/meta.pb.h"
#include "meta/meta_migrate.h"

namespace noah {
namespace meta {

std::string Node2String(const Node& node);
void String2Node(const std::string& ip_and_port, Node* node);

struct NodeInfo {
  NodeInfo() : last_alive_time(0) {
  }
  explicit NodeInfo(uint64_t update) : last_alive_time(update) {}

  uint64_t last_alive_time;
};

struct TableInfo {
  TableInfo() : id(0), master(false) {}

  bool operator < (const TableInfo& other) const {
    if (id != other.id) {
      return id < other.id;
    }
    return master < other.master;
  }

  int32_t       id;
  bool          master;
};

typedef std::unordered_map<std::string, Table> TableMap;
typedef std::unordered_map<std::string, std::set<std::string> > NodeTableMap;
typedef std::unordered_map<std::string, NodeInfo> NodeInfoMap;
typedef google::protobuf::RepeatedPtrField<Partition> RepeatedPartition;
typedef std::vector<std::string> StringVector;

void SetNodeTable(const Table& table, NodeTableMap* node_table);
void DropNodeTable(const Table& table, NodeTableMap* node_table);

struct node_less {
  bool operator()(const Node& node1, const Node& node2) const {
    if (node1.ip() != node2.ip()) {
      return node1.ip() < node2.ip();
    }
    return node1.port() < node2.port();
  }
};

struct node_equal {
  bool operator()(const Node& node1, const Node& node2) const {
    return node1.ip() == node2.ip() && node1.port() == node2.port();
  }
};

struct VersionInfo {
  VersionInfo() : epoch(0), version(0) {
  }
  explicit VersionInfo(const Version& info)
    : epoch(info.epoch()), version(info.version()) {
  }
  VersionInfo(int64_t e, int64_t v) : epoch(e), version(v) {
  }
  std::string DebugDump();
  bool operator != (const VersionInfo& cmp) const {
    return cmp.epoch != epoch || cmp.version != version;
  }

  int64_t   epoch;
  int64_t   version;
};

struct MetaInfoSnapshot {
  VersionInfo   version;
  TableMap      table_info;
  NodeTableMap  node_table;
  NodeInfoMap   nodes;

  bool CheckExpiredNodes(std::vector<std::string>* expired_nodes);
  void GetAliveNodes(std::set<Node, node_less>* alive_nodes) const;
};

class MetaInfo {
 public:
  MetaInfo();
  ~MetaInfo();

  VersionInfo version();

  bool Init();
  // Set operations, return big then 0 is right
  void RefreshNodes();
  VersionInfo UpdateNode(const Node& node);
  VersionInfo CreateTable(const VersionInfo& info, const Table& table);
  VersionInfo DownNodes(int64_t epoch, const std::vector<Node>& nodes,
                    MigrateTaskArray* tasks);
  VersionInfo DropTable(const VersionInfo& info, const StringVector& tables);
  VersionInfo CommitMigrate(const VersionInfo& info, const Processor& processor);
  bool UpdatePartNodeMS(const Node& node);
  // Get operations
  bool ExistTable(const std::string& table_name);

  bool Changed(const VersionInfo& info);
  bool LoadFromSnapshot(MetaInfoSnapshot* snapshot);
  void GetSnapshot(MetaInfoSnapshot* snapshot);

 private:
  bool Initialized();
  void GenerateMigrateTasks(const std::vector<Node>& nodes, MigrateTaskArray* tasks);
  void MigrateFromNode(const Node& node, MigrateTaskArray* tasks);
  void MigrateFromTable(const Node& node, Table* table, MigrateTaskArray* tasks);

  butil::RWMutex  meta_rw_;
  // -1 for uninitialized
  // 0 initial but no table
  VersionInfo     version_;
  // table => Table info
  TableMap        table_info_;
  // node => tables
  NodeTableMap    node_table_;
  // node => node info
  NodeInfoMap     nodes_;
};

}  // namespace meta
}  // namespace noah

#endif
