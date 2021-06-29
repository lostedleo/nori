// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_SERVER_H_
#define NOAH_NODE_SERVER_H_

#include <string>
#include <atomic>

#include "braft/repeated_timer_task.h"
#include "butil/synchronization/rw_mutex.h"
#include "butil/threading/simple_thread.h"
#include "butil/memory/scoped_ptr.h"

#include "common/meta_client.h"
#include "common/channels.h"
#include "common/entity.h"
#include "common/util.h"

#include "node/node_data.h"

#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/write_batch.h"

namespace noah {
namespace meta {
class MetaCmdRequest;
class MetaCmdResponse;
}
}

namespace noah {
namespace node {

class NodeServer;
class CreateTableRequest;
class CreateTableResponse;
class GetRequest;
class GetResponse;
class SetRequest;
class SetResponse;
class DelRequest;
class DelResponse;
class ListRequest;
class ListResponse;
class MigrateRequest;
class MigrateResponse;

typedef noah::common::TableMap TableMap;
typedef noah::common::VersionInfo VersionInfo;

class ServerTimer : public braft::RepeatedTimerTask {
 public:
  ServerTimer();
  virtual ~ServerTimer();
  int Init(NodeServer* node_server, int timeout_ms);
  virtual void run() = 0;

 protected:
  void on_destroy() override;

  NodeServer* node_server_;
};

class PingTimer : public ServerTimer {
 public:
  void run() override;
};

class ExpiredTimer : public ServerTimer {
 public:
  void run() override;
};

class StatisticsTimer : public ServerTimer {
 public:
  void run() override;
};

class MetaUpdater : public butil::DelegateSimpleThread::Delegate {
 public:
  MetaUpdater(NodeServer* node_server, const VersionInfo& info);
  virtual ~MetaUpdater() { }
  void Run() override;

 private:
  NodeServer* node_server_;
  VersionInfo info_;
};

class DropTabler : public butil::DelegateSimpleThread::Delegate {
 public:
  DropTabler(NodeServer* node_server, TableMap* tables);
  virtual ~DropTabler() { }
  void Run() override;

 private:
  NodeServer* node_server_;
  TableMap    tables_;
};

struct NodeServerOptions {
  NodeServerOptions() {
    ping_interval_ms  = 1000;
    expired_interval_ms = 3600 * 1000;
    statistics_interval_ms = 10 * 1000;
    node_timeout_ms = 1000;
    port              = 0;
  }

  std::string     meta_group;
  std::string     meta_conf;
  int32_t         ping_interval_ms;
  int32_t         expired_interval_ms;
  int32_t         statistics_interval_ms;
  int32_t         node_timeout_ms;
  std::string     ip;
  int32_t         port;
};

struct TableInfo {
  TableInfo() : partition_num(1), duplicate_num(2), capacity(0) {
  }
  explicit TableInfo(const std::string& name)
    : table_name(name), partition_num(1), duplicate_num(2), capacity(0) {
  }

  std::string table_name;
  int32_t     partition_num;
  int32_t     duplicate_num;
  int64_t     capacity;
};

class ThreadPool  {
 public:
  ThreadPool(const std::string& name_prefix, int num_threads)
    : is_active_(true), pool_(name_prefix, num_threads) {
  }

  void AddWork(butil::DelegateSimpleThreadPool::Delegate* work) {
    butil::ReaderAutoLock lock_r(&lock_);
    if (is_active_) {
      pool_.AddWork(work);
    } else {
      if (work) {
        delete work;
      }
    }
  }

  void Start() {
    butil::ReaderAutoLock lock_r(&lock_);
    if (is_active_) {
      pool_.Start();
    }
  }

  void JoinAll() {
    butil::WriterAutoLock lock_w(&lock_);
    if (is_active_) {
      pool_.JoinAll();
      is_active_ = false;
    }
  }

 private:
  butil::RWMutex  lock_;
  bool            is_active_;
  butil::DelegateSimpleThreadPool pool_;
};

class NodeServer {
 public:
  typedef noah::common::Node Node;
  typedef noah::common::Table Table;
  typedef noah::common::Partition Partition;
  typedef noah::common::MetaClient MetaClient;
  typedef noah::common::ChannelPtr ChannelPtr;
  typedef noah::common::Channels Channels;
  typedef noah::common::MetaInfo MetaInfo;
  typedef ::google::protobuf::Map<std::string, std::string> GoogleMap;
  typedef ::google::protobuf::RepeatedPtrField< ::std::string> GoogleRepeated;

  enum ServerState{
    kStart,
    kRun,
    kStop
  };

  explicit NodeServer(const NodeServerOptions& options);
  ~NodeServer();

  int Start();
  void ShutDown();

  // rpc method
  void CreateTable(const CreateTableRequest* request, CreateTableResponse* response);
  void Get(const GetRequest* request, GetResponse* response);
  void Set(const SetRequest* request, SetResponse* response);
  void Del(const DelRequest* request, DelResponse* response);
  void List(const ListRequest* request, ListResponse* response);
  void Migrate(const MigrateRequest* request, MigrateResponse* response);

  // timer func
  void HandlePing();
  void CheckExpired();
  void HandleStatistics();

  bool UpdateMetaInfo(const VersionInfo& info);
  void DropTable(const TableArray& tables);
  bool FindTable(const std::string& table_name, Table* table);

  bool LdbSet(const std::string table_name, int32_t id, const GoogleMap& key_values,
              int32_t expired);
  bool LdbDel(const std::string table_name, int32_t id, const GoogleRepeated& keys);
  bool LdbGet(const std::string& table_name, int32_t id, const GoogleRepeated& keys,
              GoogleRepeated* values);
  bool LoadDataFormDb();
  void CheckLdbExpired();
  void StatisticsTableSize();

 private:
  friend class NodeMigrate;

  bool CheckEpoch(int64_t epoch, int64_t version, const std::string& table_name,
                  int32_t id, Partition* snap_info, bool updated = false);
  VersionInfo CreateTable(const VersionInfo& info, const TableInfo& table_info);
  bool CheckShouldUpdate(const VersionInfo& info);

  NodeServerOptions     options_;
  Node                  self_node_;
  scoped_ptr<Channels>  channels_;
  MetaClient            meta_client_;
  PingTimer             ping_timer_;
  ExpiredTimer          expired_time_;
  StatisticsTimer       statistics_timer_;
  butil::RWMutex        meta_lock_;
  MetaInfo              meta_info_;
  ThreadPool            update_pool_;
  ThreadPool            migrate_pool_;
  ThreadPool            bg_pool_;
  NodeData              node_data_;

  int server_state_;
  int db_num_;
  leveldb::DB**  db_;
  leveldb::Options* db_options_;
  leveldb::WriteOptions write_options_;
  leveldb::ReadOptions read_options_;
  std::string db_path_;
};

}  // namespace node
}  // namespace noah

#endif
