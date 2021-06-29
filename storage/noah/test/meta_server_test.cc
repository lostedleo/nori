// Author: Zhenwei Zhu losted.leo@gmail.com


#include <string>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "butil/at_exit.h"
#include "butil/files/file_path.h"
#include "butil/file_util.h"
#include "bthread/bthread.h"
#include "bthread/countdown_event.h"
#include "brpc/server.h"
#include "braft/raft.h"

#include "common/channels.h"
#include "meta/meta_server.h"
#include "meta/meta_service.h"

namespace noah {
namespace meta {

class ExpectClosure : public google::protobuf::Closure {
 public:
  void Run() {
    if (_cond) {
      _cond->signal();
    }
    delete this;
  }
  ExpectClosure(bthread::CountdownEvent* cond, const char* pos)
    : _cond(cond), _pos(pos) {}

  bthread::CountdownEvent* _cond;
  const char* _pos;
};

typedef ExpectClosure ApplyClosure;

#define NEW_APPLYCLOSURE(arg...) \
        (new ExpectClosure(arg, __FILE__ ":" BAIDU_SYMBOLSTR(__LINE__)))

class Cluster {
 public:
  Cluster(const std::string& name, const std::vector<braft::PeerId>& peers,
          int32_t election_timeout_ms = 300)
    : name_(name), peers_(peers)
      , election_timeout_ms_(election_timeout_ms) {
  }
  ~Cluster() {
    StopAll();
  }

  int Start(const butil::EndPoint& listen_addr, bool emptypeers_ = false,
      int snapshot_interval_s = 30) {
    butil::AutoLock lock(lock_);
    if (server_map_[listen_addr] == NULL) {
      brpc::Server* server = new brpc::Server();
      MetaServer* meta_server = new MetaServer();
      MetaServiceImpl* meta_service = new MetaServiceImpl(meta_server);

      if (0 != server->AddService(meta_service, brpc::SERVER_DOESNT_OWN_SERVICE)) {
        LOG(ERROR) << "Failed to add service";
        delete server;
        delete meta_server;
        delete meta_service;
        return -1;
      }

      if (braft::add_service(server, listen_addr) != 0
          || server->Start(listen_addr, NULL) != 0) {
        LOG(ERROR) << "Failed to start raft service";
        delete server;
        delete meta_service;
        delete meta_server;
        return -1;
      }

      braft::NodeOptions options;
      options.election_timeout_ms = election_timeout_ms_;
      options.snapshot_interval_s = snapshot_interval_s;
      if (!emptypeers_) {
        options.initial_conf = braft::Configuration(peers_);
      }
      options.fsm = meta_server;
      butil::string_printf(&options.log_uri, "local://./data/%s/log",
          butil::endpoint2str(listen_addr).c_str());
      butil::string_printf(&options.raft_meta_uri, "local://./data/%s/raft_meta",
          butil::endpoint2str(listen_addr).c_str());
      butil::string_printf(&options.snapshot_uri, "local://./data/%s/snapshot",
          butil::endpoint2str(listen_addr).c_str());

      if (0 != meta_server->Start(listen_addr, options)) {
        LOG(ERROR) << "Failed to start MetaServer" << listen_addr;
        delete server;
        delete meta_service;
        delete meta_server;
        return -1;
      }

      server_map_[listen_addr] = server;
      meta_server_map_[listen_addr] = meta_server;
      meta_service_map_[listen_addr] = meta_service;
    }
    return 0;
  }

  int Stop(const butil::EndPoint& listen_addr) {
    butil::AutoLock lock(lock_);
    noah::meta::MetaServer* meta_server = meta_server_map_[listen_addr];
    if (meta_server) {
      meta_server->ShutDown();
      server_map_[listen_addr]->Stop(0);
      meta_server->Join();
      server_map_[listen_addr]->Join();

      delete server_map_[listen_addr];
      server_map_.erase(listen_addr);
      delete meta_service_map_[listen_addr];
      meta_service_map_.erase(listen_addr);
      delete meta_server;
      meta_server_map_.erase(listen_addr);

      return 0;
    }
    return -1;
  }

  void StopAll() {
    std::vector<butil::EndPoint> addrs;
    for (const auto& value : server_map_) {
      addrs.push_back(value.first);
    }
    for (const auto& value : addrs) {
      Stop(value);
    }
  }

  MetaServer* GetByAddr(const butil::EndPoint& listen_addr) {
    butil::AutoLock lock(lock_);
    return meta_server_map_[listen_addr];
  }

  void Clean(const butil::EndPoint& listen_addr) {
    std::string data_path;
    butil::string_printf(&data_path, "./data/%s",
        butil::endpoint2str(listen_addr).c_str());

    if (!butil::DeleteFile(butil::FilePath(data_path), true)) {
      LOG(ERROR) << "delete path failed, path: " << data_path;
    }
  }

  MetaServer* Leader() {
    butil::AutoLock lock(lock_);
    MetaServer* node = NULL;
    for (const auto& value : meta_server_map_) {
      if (value.second->IsLeader()) {
        node = value.second;
        break;
      }
    }
    return node;
  }

  void Followers(std::vector<MetaServer*>* nodes) {
    nodes->clear();
    butil::AutoLock lock(lock_);

    for (const auto& value : meta_server_map_) {
      if (!value.second->IsLeader()) {
        nodes->push_back(value.second);
      }
    }
  }

  void WaitLeader() {
    while (true) {
      MetaServer* node = Leader();
      if (node) {
        return;
      } else {
        usleep(100 * 1000);
      }
    }
  }

  void EnsureLeader(const butil::EndPoint& expect_addr) {
CHECK:
    butil::AutoLock lock(lock_);
    for (const auto& value : meta_server_map_) {
      braft::PeerId leader_id = value.second->Leader();
      if (leader_id.addr != expect_addr) {
        goto WAIT;
      }
    }
    return;

WAIT:
    usleep(100 * 1000);
    goto CHECK;
  }

 private:
  butil::AtExitManager exit_manager_;
  butil::Lock lock_;
  std::string name_;
  std::vector<braft::PeerId> peers_;
  std::map<butil::EndPoint, brpc::Server*> server_map_;
  std::map<butil::EndPoint, noah::meta::MetaServer*> meta_server_map_;
  std::map<butil::EndPoint, noah::meta::MetaServiceImpl*> meta_service_map_;
  int32_t election_timeout_ms_;
};

class MetaTest : public testing::Test {
 protected:
  void SetUp() {
    system("rm -rf ./data");
  }

  void TearDown() {
    system("rm -rf ./data");
  }
};

TEST_F(MetaTest, SingleMeta) {
  std::vector<braft::PeerId> peers;
  for (int i = 0; i < 1; ++i) {
    braft::PeerId peer;
    peer.addr.ip = butil::my_ip();
    peer.addr.port = 5006 + i;
    peer.idx = 0;

    peers.push_back(peer);
  }
  Cluster cluster("unittest", peers);
  for (const auto& peer : peers) {
    ASSERT_EQ(0, cluster.Start(peer.addr));
  }

  cluster.WaitLeader();
  MetaServer* leader = cluster.Leader();
  CHECK(leader);
}

TEST_F(MetaTest, TripleMeta) {
  std::vector<braft::PeerId> peers;
  std::vector<noah::common::Node> nodes;
  noah::common::Node node;
  for (int i = 0; i < 3; ++i) {
    braft::PeerId peer;
    peer.addr.ip = butil::my_ip();
    peer.addr.port = 5006 + i;
    peer.idx = 0;
    node.ip = butil::my_ip_cstr();
    node.port = 5006 +i;
    nodes.push_back(node);

    peers.push_back(peer);
  }
  Cluster cluster("unittest", peers);
  for (const auto& peer : peers) {
    ASSERT_EQ(0, cluster.Start(peer.addr, false, 3));
  }

  cluster.WaitLeader();
  MetaServer* leader = cluster.Leader();
  CHECK(leader);

  // Test for Ping
  bthread::CountdownEvent cond(1);
  MetaCmdRequest ping_request;
  MetaCmdResponse ping_response;
  ping_request.set_type(Type::PING);
  ping_request.mutable_version()->set_epoch(0);
  ping_request.mutable_version()->set_version(0);
  ping_request.mutable_ping()->mutable_node()->set_ip(butil::my_ip_cstr());
  ping_request.mutable_ping()->mutable_node()->set_port(5700);

  leader->Apply(&ping_request, &ping_response, NEW_APPLYCLOSURE(&cond));
  cond.wait();
  ASSERT_EQ(Status::OK, ping_response.status());

  cond.reset(1);
  ping_request.mutable_ping()->mutable_node()->set_port(5701);
  leader->Apply(&ping_request, &ping_response, NEW_APPLYCLOSURE(&cond));
  cond.wait();
  ASSERT_EQ(Status::OK, ping_response.status());

  cond.reset(1);
  ping_request.mutable_ping()->mutable_node()->set_port(5702);
  leader->Apply(&ping_request, &ping_response, NEW_APPLYCLOSURE(&cond));
  cond.wait();
  ASSERT_EQ(Status::OK, ping_response.status());

  sleep(5);

  // Test for Init
  MetaCmdRequest init_request;
  MetaCmdResponse init_response;
  init_request.set_type(Type::INIT);
  init_request.mutable_version()->set_epoch(ping_response.version().epoch());
  init_request.mutable_version()->set_version(ping_response.version().version());
  init_request.mutable_init()->mutable_table()->set_name("table1");
  for (int i = 0; i < 3; ++i) {
    Partition* partition = init_request.mutable_init()->mutable_table()->add_partitions();
    partition->set_id(i);
    partition->set_status(PStatus::ACTIVE);
    partition->mutable_master()->set_ip(butil::my_ip_cstr());
    partition->mutable_master()->set_port(5700 + i);

    for (int j = 0; j < 2; ++j) {
      Node * node = partition->add_slaves();
      node->set_ip(butil::my_ip_cstr());
      node->set_port(5701 + i + j);
    }
  }
  cond.reset(1);
  leader->Apply(&init_request, &init_response, NEW_APPLYCLOSURE(&cond));
  cond.wait();
  ASSERT_EQ(Status::OK, init_response.status());

  sleep(5);

  // Table exist could not Create Table again
  cond.reset(1);
  init_request.mutable_version()->set_epoch(init_response.version().epoch());
  init_request.mutable_version()->set_version(init_response.version().version());
  leader->Apply(&init_request, &init_response, NEW_APPLYCLOSURE(&cond));
  cond.wait();
  ASSERT_EQ(Status::EXIST, init_response.status());

  init_request.mutable_init()->mutable_table()->set_name("table2");
  cond.reset(1);
  leader->Apply(&init_request, &init_response, NEW_APPLYCLOSURE(&cond));
  cond.wait();
  ASSERT_EQ(Status::OK, init_response.status());

  // Test for Pull
  {
    MetaCmdRequest pull_request;
    MetaCmdResponse pull_response;
    pull_request.set_type(Type::PULL);
    pull_request.mutable_version()->set_epoch(init_response.version().epoch());
    pull_request.mutable_version()->set_version(init_response.version().version());
    pull_request.mutable_pull()->mutable_node()->set_ip(butil::my_ip_cstr());
    pull_request.mutable_pull()->mutable_node()->set_port(5702);
    cond.reset(1);
    leader->Apply(&pull_request, &pull_response, NEW_APPLYCLOSURE(&cond));
    cond.wait();
    ASSERT_EQ(Status::OK, pull_response.status());
  }

  {
    MetaCmdRequest pull_request;
    MetaCmdResponse pull_response;
    pull_request.set_type(Type::PULL);
    pull_request.mutable_version()->set_epoch(init_response.version().epoch());
    pull_request.mutable_version()->set_version(init_response.version().version());
    pull_request.mutable_pull()->add_names("table2");
    cond.reset(1);
    leader->Apply(&pull_request, &pull_response, NEW_APPLYCLOSURE(&cond));
    cond.wait();
    ASSERT_EQ(Status::OK, pull_response.status());
  }

  // Test for ListTable
  {
    MetaCmdRequest list_request;
    MetaCmdResponse list_response;
    list_request.set_type(Type::LISTTABLE);
    list_request.mutable_version()->set_epoch(init_response.version().epoch());
    list_request.mutable_version()->set_version(init_response.version().version());
    cond.reset(1);
    leader->Apply(&list_request, &list_response, NEW_APPLYCLOSURE(&cond));
    cond.wait();
    ASSERT_EQ(Status::OK, list_response.status());
  }

  // Test for ListMeta
  {
    MetaCmdRequest list_request;
    MetaCmdResponse list_response;
    list_request.set_type(Type::LISTMETA);
    list_request.mutable_version()->set_epoch(init_response.version().epoch());
    list_request.mutable_version()->set_version(init_response.version().version());
    cond.reset(1);
    leader->Apply(&list_request, &list_response, NEW_APPLYCLOSURE(&cond));
    cond.wait();
    ASSERT_EQ(Status::OK, list_response.status());
  }

  // Test for ListNode
  {
    MetaCmdRequest list_request;
    MetaCmdResponse list_response;
    list_request.set_type(Type::LISTNODE);
    list_request.mutable_version()->set_epoch(init_response.version().epoch());
    list_request.mutable_version()->set_version(init_response.version().version());
    cond.reset(1);
    leader->Apply(&list_request, &list_response, NEW_APPLYCLOSURE(&cond));
    cond.wait();
    ASSERT_EQ(Status::OK, list_response.status());
  }

  {
    noah::common::ChannelOptions options;
    options.timeout_ms = 1000;
    options.max_retry = 3;
    options.protocol = "baidu_std";
    noah::common::Channels channels(options);
    ParallelChannelPtr parallel_channel = channels.GetParallelChannelByNodes(nodes);
    brpc::Controller cntl;
    google::protobuf::RpcChannel* channel =
        reinterpret_cast<google::protobuf::RpcChannel*>(parallel_channel.get());
    MetaService_Stub stub(channel);

    MetaCmdRequest list_request;
    MetaCmdResponse list_response;
    list_request.set_type(Type::LISTMETA);
    list_request.mutable_version()->set_epoch(init_response.version().epoch());
    list_request.mutable_version()->set_version(init_response.version().version());
    cond.reset(1);
    stub.MetaCmd(&cntl, &list_request, &list_response, NEW_APPLYCLOSURE(&cond));
    cond.wait();
    if (cntl.Failed()) {
      LOG(ERROR) << "Request failed " << cntl.ErrorText();
    }
    LOG(INFO) << "parallel list response " << list_response.DebugString();
    sleep(3);
  }
}

}  // namespace meta
}  // namespace noah
