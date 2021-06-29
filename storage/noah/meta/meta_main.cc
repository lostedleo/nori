// Author: Zhenwei Zhu losted.leo@gmail.com

#include <gflags/gflags.h>

#include "butil/logging.h"
#include "butil/at_exit.h"
#include "brpc/server.h"
#include "braft/raft.h"

#include "meta/meta_server.h"
#include "meta/meta_service.h"

#include "common/util.h"

DEFINE_string(ip, "", "Server listen ip of this peer");
DEFINE_int32(port, 8100, "Listen port of this peer");
DEFINE_int32(internal_port, -1, "Only allow builtin services at this port");
DEFINE_bool(disable_cli, false, "Don't allow raft_cli access this node");
DEFINE_int32(election_timeout_ms, 5000,
            "Start election in such milliseconds if disconnect with the leader");
DEFINE_int32(snapshot_interval, 30, "Interval between each snapshot");
DEFINE_string(data_path, "./data", "Path of data stored on");
DEFINE_string(conf, "", "Initial configuration of the replication group");
DEFINE_string(eth, "eth1", "Init eth1 get localhost ip");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  if (FLAGS_conf.empty()) {
    LOG(ERROR) << "MetaServer conf is empty()";
    return -1;
  }

  butil::AtExitManager exit_manager;
  brpc::Server server;
  noah::meta::MetaServer meta_server;
  noah::meta::MetaServiceImpl service(&meta_server);

  // Add service into RPC server
  if (server.AddService(&service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Failed to add servcie";
    return -1;
  }
  // raft can share the same RPC server
  if (braft::add_service(&server, FLAGS_port) != 0) {
    LOG(ERROR) << "Failed to add raft service";
    return -1;
  }
  brpc::ServerOptions options;
  options.internal_port = FLAGS_internal_port;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Failed to start Server";
    return -1;
  }

  // Start MetaServer
  butil::ip_t ip;
  std::string ipStr;
  if (FLAGS_ip.empty()) {
    if (!noah::common::GetIpAddress(&ipStr, FLAGS_eth)) {
      LOG(ERROR) << "Get ip address failed!";
      return -1;
    }

    if (0 != butil::str2ip(ipStr.c_str(), &ip)) {
      LOG(ERROR) << "Parse strip failed! " << ipStr;
      return -1;
    }
  } else {
    if (0 != butil::str2ip(FLAGS_ip.c_str(), &ip)) {
      LOG(ERROR) << "Error ip " << FLAGS_ip;
      return -1;
    }
  }

  butil::EndPoint addr(ip, FLAGS_port);
  braft::NodeOptions node_options;
  if (node_options.initial_conf.parse_from(FLAGS_conf) != 0) {
    LOG(ERROR) << "Failed to parse configuration \'" << FLAGS_conf << '\'';
    return -1;
  }
  node_options.election_timeout_ms = FLAGS_election_timeout_ms;
  node_options.fsm = &meta_server;
  node_options.node_owns_fsm = false;
  node_options.snapshot_interval_s = FLAGS_snapshot_interval;
  std::string prefix = "local://" + FLAGS_data_path;
  node_options.log_uri = prefix + "/log";
  node_options.raft_meta_uri = prefix + "/raft_meta";
  node_options.snapshot_uri = prefix + "/snapshot";
  node_options.disable_cli = FLAGS_disable_cli;
  if (meta_server.Start(addr, node_options) != 0) {
    LOG(ERROR) << "Failed to start MetaServer";
    return -1;
  }

  LOG(INFO) << "Meta service is running on " << server.listen_address();
  // Wait until 'CTRL-C' is pressed. then Stop() and Join() the service
  while (!brpc::IsAskedToQuit()) {
    sleep(1);
  }

  LOG(INFO) << "Meta service is going to quit";
  meta_server.ShutDown();
  server.Stop(0);

  // Wait until all the processing tasks are over
  meta_server.Join();
  server.Join();

  return 0;
}

