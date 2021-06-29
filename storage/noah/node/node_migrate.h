// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_MIGRATE_H_
#define NOAH_NODE_MIGRATE_H_

#include "common/entity.h"

#include <memory>
#include <map>
#include <set>

#include "butil/threading/simple_thread.h"
#include "butil/synchronization/rw_mutex.h"

namespace noah {
namespace meta {
class MigrateUnit;
}
}

namespace noah {
namespace node {
class NodeServer;
class Cache;
class MigrateRequest;
class MigrateResponse;

typedef noah::common::Node Node;
typedef noah::common::Role Role;

struct MigrateTaskInfo {
  MigrateTaskInfo() : partition_id(0) {
  }
  explicit MigrateTaskInfo(const noah::meta::MigrateUnit& unit);
  void Convert2MigrateUnit(noah::meta::MigrateUnit* unit);

  std::string   table_name;
  int32_t       partition_id;
  Node          fault;
  Node          src;
  Node          dst;
  Role          role;
};

typedef std::set<Node> NodeSet;
typedef std::map<std::string, NodeSet> StringNodeMap;
typedef std::shared_ptr<Cache> CachePtr;

class NodeMigrate : public butil::DelegateSimpleThread::Delegate {
 public:
  NodeMigrate(const MigrateTaskInfo& task, NodeServer* node_server);
  virtual ~NodeMigrate() { }
  void Run() override;
  void Cancel();
  static bool CheckMigrating(const std::string& table_name, int32_t id,
                             NodeSet* nodes);

 private:
  void RegisterMigrateInfo();
  bool MigrateData();
  void UnregisterMigrateInfo();
  bool MigrateToDst(MigrateRequest* request, MigrateResponse* response);
  bool CommitToMeta();

  bool                  canceled_;
  MigrateTaskInfo       task_info_;
  CachePtr              cache_;
  NodeServer*           node_server_;
  static butil::RWMutex info_lock_;
  static StringNodeMap  migrating_info_;
};
}  // namespace node
}  // namespace noah
#endif
