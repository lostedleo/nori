// auth Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_META_SERVER_H_
#define NOAH_META_SERVER_H_

#include <atomic>

#include "butil/memory/scoped_ptr.h"
#include "braft/raft.h"
#include "braft/repeated_timer_task.h"

#include "meta/meta_info.h"
#include "meta/meta_command.h"

namespace noah {
namespace meta {
class MetaServer;

class MetaClosure;
class MetaCmdRequest;
class MetaCmdResponse;

class ServerTimer : public braft::RepeatedTimerTask {
 public:
  ServerTimer();
  virtual ~ServerTimer();
  int Init(MetaServer* meta_server, int timeout_ms);
  virtual void run() = 0;

 protected:
  void on_destroy() override;

  MetaServer* meta_server_;
};

class ScheduleTimer : public ServerTimer {
 public:
  void run() override;
};

class MetaServer : public braft::StateMachine {
 public:
  enum MetaOpType {
    OP_UNKNOWN = 0,
    OP_GETMETA = 1,
    OP_GETNODE = 2,
  };
  MetaServer();
  ~MetaServer();

  int Start(const butil::EndPoint& addr, const braft::NodeOptions& options);
  bool IsLeader();
  braft::PeerId Leader();
  void ShutDown();
  void Join();
  void Apply(const MetaCmdRequest* request, MetaCmdResponse* response,
             google::protobuf::Closure* done);

 private:
  friend class MetaClosure;
  friend class MetaCommand;
  friend class ScheduleTimer;

  void Redirect(MetaCmdResponse* response);
  bool ListMeta(MetaNodes* meta_nodes);
  void DoScheduleTask();
  void CheckNodesStatus();

  // override StateMachine virtual functions
  void on_apply(braft::Iterator& iter) override;
  void on_shutdown() override;
  void on_snapshot_save(braft::SnapshotWriter* writer,
                        braft::Closure* done) override;
  int on_snapshot_load(braft::SnapshotReader* reader) override;
  void on_leader_start(int64_t term) override;
  void on_leader_stop(const butil::Status& status) override;
  void on_error(const braft::Error& e) override;
  void on_configuration_committed(const braft::Configuration& conf) override;
  void on_stop_following(const braft::LeaderChangeContext& ctx) override;
  void on_start_following(const braft::LeaderChangeContext& ctx) override;

  static void* SaveSnapshot(void* arg);

  struct SnapshotClosure {
    MetaInfo*               meta_info;
    braft::SnapshotWriter*  writer;
    braft::Closure*         done;
  };

  MetaMigrate             meta_migrate_;
  MetaInfo                meta_info_;
  MetaCommand             meta_command_;
  scoped_ptr<braft::Node> node_;
  butil::atomic<int64_t>  leader_term_;
  ScheduleTimer           schedule_timer_;
};
}  // namespace meta
}  // namespace noah

#endif
