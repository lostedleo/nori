// auth Zhenwei Zhu losted.leo@gmail.com

#include "meta/meta_server.h"

#include <fstream>

#include <gflags/gflags.h>

#include "bthread/bthread.h"
#include "brpc/server.h"
#include "butil/logging.h"
#include "butil/endpoint.h"
#include "braft/util.h"
#include "braft/storage.h"
#include "braft/protobuf_file.h"

#include "common/wait_closure.h"

#include "meta/meta.pb.h"
#include "meta/meta_closure.h"

DEFINE_bool(check_term, true, "Check if the leader changed to another term");
DEFINE_bool(log_applied_task, false, "Print notice log when a task is applied");
DEFINE_string(group, "MetaServer", "Id of the replication group");
DEFINE_int32(timer_interval_ms, 1000, "The interval ms to do schedule task");
DEFINE_bool(auto_migrate, false, "Auto migrate table partitions");

namespace noah {
namespace meta {
const char kEpoch[] = "version";
const char kTableInfo[] = "tables";
const char kNodeInfo[] = "nodes";

typedef google::protobuf::Map<std::string, Table> GoogleTableMap;
typedef google::protobuf::RepeatedPtrField<std::string> GoogleRepeatedString;

void ConvertTableSnap(const TableMap& table_map, TableSnap* table_snap) {
  GoogleTableMap* table_info = table_snap->mutable_table_info();
  for (const auto& value : table_map) {
    (*table_info)[value.first] = value.second;
  }
}

void ConvertNodeSnap(const NodeInfoMap& node_map, NodeSnap* node_snap) {
  for (const auto& value : node_map) {
    NodeStatus* node_status = node_snap->add_nodes();
    Node* node = node_status->mutable_node();
    String2Node(value.first, node);
    node_status->set_status(value.second.last_alive_time == 0? NodeState::DOWN:NodeState::UP);
  }
}

void SetMetaInfoSnapshot(const NodeSnap& node_snap, const TableSnap& table_snap,
                         MetaInfoSnapshot* snapshot) {
  const GoogleTableMap& table_map = table_snap.table_info();
  for (const auto& value : table_map) {
    snapshot->table_info[value.first] = value.second;
    SetNodeTable(value.second, &snapshot->node_table);
  }

  size_t size = node_snap.nodes_size();
  for (size_t i = 0; i < size; i++) {
    auto node_status = node_snap.nodes(i);
    std::string node_str = Node2String(*node_status.mutable_node());
    snapshot->nodes[node_str] = NodeInfo(node_status.status() ==  NodeState::DOWN ?
        0 : butil::gettimeofday_us());
  }
}

ServerTimer::ServerTimer() : meta_server_(NULL) {
}

ServerTimer::~ServerTimer() {
}

int ServerTimer::Init(MetaServer* meta_server, int timeout_ms) {
  if (0 != RepeatedTimerTask::init(timeout_ms)) {
    LOG(ERROR) << "ServerTimer init error";
    return -1;
  }
  meta_server_ = meta_server;
  return 0;
}

void ServerTimer::on_destroy() {
  meta_server_ = NULL;
}

void ScheduleTimer::run() {
  if (meta_server_) {
    meta_server_->DoScheduleTask();
  }
}

MetaServer::MetaServer() :
  meta_command_(this), node_(nullptr), leader_term_(-1) {
}

MetaServer::~MetaServer() {
}

int MetaServer::Start(const butil::EndPoint& addr,
                      const braft::NodeOptions& options) {
  scoped_ptr<braft::Node> node(new braft::Node(FLAGS_group, braft::PeerId(addr)));
  if (node->init(options) != 0) {
    LOG(ERROR) << "Failed to init raft node";
    return -1;
  }
  node_.reset(node.release());
  meta_info_.Init();
  if (0 != schedule_timer_.Init(this, FLAGS_timer_interval_ms)) {
    LOG(ERROR) << "Failed to init schedule timer";
    return -1;
  }
  schedule_timer_.start();
  return 0;
}

bool MetaServer::IsLeader() {
  return leader_term_.load(butil::memory_order_acquire) > 0;
}

braft::PeerId MetaServer::Leader() {
  return node_->leader_id();
}

void MetaServer::ShutDown() {
  schedule_timer_.destroy();
  if (node_.get()) {
    node_->shutdown(NULL);
  }
}

void MetaServer::Join() {
  if (node_.get()) {
    node_->join();
  }
}

void MetaServer::Apply(const MetaCmdRequest* request, MetaCmdResponse* response,
                       google::protobuf::Closure* done) {
  CHECK(request);
  CHECK(response);
  brpc::ClosureGuard done_guard(done);
  const int64_t term = leader_term_.load(butil::memory_order_relaxed);
  if (term < 0) {
    return Redirect(response);
  }
  // PreDealRequest to decide apply this request as a braft::Task
  if (meta_command_.PreDealRequest(request, response)) {
    return;
  }

  // Serialize request to IOBuf
  butil::IOBuf log;
  butil::IOBufAsZeroCopyOutputStream wrapper(&log);
  if (!request->SerializeToZeroCopyStream(&wrapper)) {
    LOG(ERROR) << "Failed to serialize request" << request;
    response->set_status(Status::ERROR);
    return;
  }
  // Apply this log as a braft::Task
  braft::Task task;
  task.data = &log;
  task.done = new MetaClosure(this, request, response, done_guard.release());
  if (FLAGS_check_term) {
    task.expected_term = term;
  }
  return node_->apply(task);
}

void MetaServer::Redirect(MetaCmdResponse* response) {
  response->set_status(Status::REDIRECT);
  if (node_.get()) {
    braft::PeerId leader = node_->leader_id();
    if (!leader.is_empty()) {
      response->set_redirect(leader.to_string());
    }
  }
}

bool MetaServer::ListMeta(MetaNodes* meta_nodes) {
  std::vector<braft::PeerId> peers;
  butil::Status status = node_->list_peers(&peers);
  if (!status.ok()) {
    return false;
  }
  braft::PeerId leader = node_->leader_id();
  meta_nodes->mutable_leader()->set_ip(butil::ip2str(leader.addr.ip).c_str());
  meta_nodes->mutable_leader()->set_port(leader.addr.port);
  for (const auto& peer : peers) {
    if (peer != leader) {
      Node* node = meta_nodes->mutable_followers()->Add();
      node->set_ip(butil::ip2str(peer.addr.ip).c_str());
      node->set_port(peer.addr.port);
    }
  }

  return true;
}

void MetaServer::DoScheduleTask() {
  if (IsLeader()) {
      CheckNodesStatus();
    if (FLAGS_auto_migrate) {
      CheckNodesStatus();
    }
  }
}

void MetaServer::CheckNodesStatus() {
  MetaInfoSnapshot snapshot;
  meta_info_.GetSnapshot(&snapshot);
  std::vector<std::string> expired_nodes;
  snapshot.CheckExpiredNodes(&expired_nodes);
  if (expired_nodes.empty()) {
    return;
  }
  LOG(INFO) << "Expired nodes " << expired_nodes.size();
  MetaCmdRequest request;
  MetaCmdResponse response;
  request.mutable_version()->set_epoch(snapshot.version.epoch);
  request.mutable_version()->set_version(snapshot.version.version);
  request.set_type(Type::DOWNNODES);
  for (const auto& node : expired_nodes) {
    String2Node(node, request.mutable_down_nodes()->add_nodes());
  }

  bthread::CountdownEvent cond(1);
  Apply(&request, &response, NEW_WAITCLOSURE(&cond));
  cond.wait();
}

void MetaServer::on_apply(braft::Iterator& iter) {
  for (; iter.valid(); iter.next()) {
    braft::AsyncClosureGuard done_guard(iter.done());

    const MetaCmdRequest* request = NULL;
    MetaCmdRequest temp_request;
    MetaCmdResponse* response = NULL;
    if (iter.done()) {
      // This task is applied by this node
      MetaClosure* closure =  dynamic_cast<MetaClosure*>(iter.done());
      request = closure->request();
      response = closure->response();
    } else {
      butil::IOBufAsZeroCopyInputStream wrapper(iter.data());
      CHECK(temp_request.ParseFromZeroCopyStream(&wrapper));
    }
    request = request ? request : &temp_request;
    meta_command_.DealRequest(request, response);

    LOG_IF(INFO, FLAGS_log_applied_task) << "Deal task " << request->DebugString()
           << " response " << (response ? response->DebugString() : "not exist");
  }
}

void MetaServer::on_shutdown() {
  LOG(INFO) << "This node is down";
}

void MetaServer::on_snapshot_save(braft::SnapshotWriter* writer,
                                  braft::Closure* done) {
  SnapshotClosure* sc = new SnapshotClosure;
  sc->meta_info = &meta_info_;
  sc->writer = writer;
  sc->done = done;

  bthread_t tid;
  bthread_start_urgent(&tid, NULL, SaveSnapshot, sc);
}

int MetaServer::on_snapshot_load(braft::SnapshotReader* reader) {
  CHECK_EQ(-1, leader_term_) << "Leader is not supposed to load snapshot";
  MetaInfoSnapshot snapshot;
  // First load epoch
  std::string epoch_path = reader->get_path() + "/" + kEpoch;
  std::ifstream is(epoch_path.c_str());
  is >> snapshot.version.epoch;
  is >> snapshot.version.version;

  // Second load Table info
  std::string tables_path = reader->get_path() + "/" + kTableInfo;
  braft::ProtoBufFile table_file(tables_path);
  TableSnap table_snap;
  if (0 != table_file.load(&table_snap)) {
    LOG(ERROR) << "Failed to load snapshot from " << tables_path;
    return -1;
  }

  // Last load nodes
  std::string nodes_path = reader->get_path() + "/" + kNodeInfo;
  braft::ProtoBufFile node_file(nodes_path);
  NodeSnap node_snap;
  if (0 != node_file.load(&node_snap)) {
    LOG(ERROR) << "Failed to load snapshot from " << nodes_path;
    return -1;
  }
  SetMetaInfoSnapshot(node_snap, table_snap, &snapshot);

  meta_info_.LoadFromSnapshot(&snapshot);
  LOG(INFO) << "Load snapshot success " << snapshot.version.DebugDump();
  return 0;
}

void MetaServer::on_leader_start(int64_t term) {
  // Refresh nodes active time to check node server failure
  meta_info_.RefreshNodes();
  leader_term_.store(term, butil::memory_order_release);
  LOG(INFO) << "Node becomes leader";
}

void MetaServer::on_leader_stop(const butil::Status& status) {
  leader_term_.store(-1 , butil::memory_order_release);
  LOG(INFO) << "Node stepped down: " << status;
}

void MetaServer::on_error(const braft::Error& e) {
  LOG(ERROR) << "Met raft error " << e;
}

void MetaServer::on_configuration_committed(const braft::Configuration& conf) {
  LOG(INFO) << "Configuration of this group is " << conf;
}

void MetaServer::on_stop_following(const braft::LeaderChangeContext& ctx) {
  LOG(INFO) << "Node stops following " << ctx;
}

void MetaServer::on_start_following(const braft::LeaderChangeContext& ctx) {
  LOG(INFO) << "Node start following " << ctx;
}

void* MetaServer::SaveSnapshot(void* arg) {
  SnapshotClosure* sc = reinterpret_cast<SnapshotClosure*>(arg);
  std::unique_ptr<SnapshotClosure> sc_guard(sc);
  brpc::ClosureGuard done_guard(sc->done);

  // Save MetaInfoSnapshot
  MetaInfoSnapshot snapshot;
  sc->meta_info->GetSnapshot(&snapshot);
  // First save nodes
  std::string nodes_path = sc->writer->get_path() + "/" + kNodeInfo;
  NodeSnap node_snap;
  ConvertNodeSnap(snapshot.nodes, &node_snap);
  braft::ProtoBufFile node_file(nodes_path);
  if (0 != node_file.save(&node_snap, true))  {
    LOG(ERROR) << "Failed to save pb_file " << nodes_path;
    sc->done->status().set_error(EIO, "Failed to save pb_file");
    return NULL;
  }
  if (0 != sc->writer->add_file(kNodeInfo)) {
    LOG(ERROR) << "Failed to add file to writer " << nodes_path;
    sc->done->status().set_error(EIO, "Failed to add node_info file to writer");
    return NULL;
  }

  // Second save tables
  std::string tables_path = sc->writer->get_path() + "/" + kTableInfo;
  TableSnap table_snap;
  ConvertTableSnap(snapshot.table_info, &table_snap);
  braft::ProtoBufFile table_file(tables_path);
  if (0 != table_file.save(&table_snap, true)) {
    LOG(ERROR) << "Failed to save pb_file " << tables_path;
    sc->done->status().set_error(EIO, "Failed to save pb_file");
    return NULL;
  }
  if (0 != sc->writer->add_file(kTableInfo)) {
    LOG(ERROR) << "Failed to add file to writer " << tables_path;
    sc->done->status().set_error(EIO, "Failed to add table_info file to writer");
    return NULL;
  }

  // Last save epoch
  std::string epoch_path = sc->writer->get_path() + "/" + kEpoch;
  std::ofstream os(epoch_path.c_str());
  os << snapshot.version.epoch;
  os << "\n";
  os << snapshot.version.version;
  if (0 != sc->writer->add_file(kEpoch)) {
    LOG(ERROR) << "Failed to add file to writer " << epoch_path;
    sc->done->status().set_error(EIO, "Failed to add epoch file to writer");
    return NULL;
  }

  LOG(INFO) << "SaveSnapshot success " << snapshot.version.DebugDump();
  return NULL;
}

}  // namespace meta
}  // namespace noah
