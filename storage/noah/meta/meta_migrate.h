// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_META_MIGRATE_H_
#define NOAH_META_MIGRATE_H_

#include <string>

#include <set>
#include <vector>
#include <map>

#include "butil/synchronization/lock.h"

#include "meta/meta.pb.h"

namespace noah {
namespace meta {
struct MetaInfoSnapshot;
struct node_less;

struct PartitionNode {
  Node  node;
  Role  role;
};

typedef std::vector<PartitionNode>  PartitionNodeArray;

struct MigrateTask {
  MigrateTask() : partition_id(0) { }

  std::string         table_name;
  int32_t             partition_id;
  PartitionNode       fault_node;
};

struct Processor {
  Processor() { }
  explicit Processor(const MigrateUnit& unit);
  std::string ToString();

  MigrateTask   task;
  Node          src;
  Node          dst;
};

typedef std::vector<MigrateTask> MigrateTaskArray;
typedef std::map<std::string, MigrateTask> MigrateTaskMap;
typedef std::map<std::string, Processor> ProcessorMap;
typedef std::map<Node, int, node_less> NodeIntMap;
typedef std::map<std::string, NodeIntMap> TableInfoMap;

std::string DebugString(const MigrateTask& task);
std::string DebugString(const MigrateTaskArray& tasks);
std::string Task2String(const MigrateTask& task);
std::string ProcessorString(const std::string table_name, int32_t id, const Node& dst);

class MetaMigrate {
 public:
  MetaMigrate();
  ~MetaMigrate();

  bool HasMigrateTask();
  void AddMigrateTasks(const MigrateTaskArray& tasks);
  bool GetMigrateTask(const Node& node, const MetaInfoSnapshot* snapshot,
                      Processor* processor);
  bool CommitMigrate(const Processor& processor);

 private:
  bool FindCandidateNode(const Table& table, const Partition& partition,
                         const std::set<Node, node_less>& alive_nodes,
                         const Node& fault_node, Node* dst);
  int32_t GetMigratingNumber(const std::string& table_name, const Node& node);

  butil::Lock         lock_;
  MigrateTaskMap      migrate_tasks_;
  ProcessorMap        processor_info_;
  TableInfoMap        migrating_info_;
};

}  // namespace meta
}  // namespace noah
#endif
