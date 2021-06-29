// Author: Zhenwei Zhu losted.leo@gmail.com

#include <string>

#include <gflags/gflags.h>

#include "butil/logging.h"
#include "butil/at_exit.h"
#include "brpc/server.h"
#include "butil/memory/scoped_ptr.h"

#include "node/node.pb.h"
#include "node/cache.h"
#include "node/node_service.h"
#include "node/node_server.h"
#include "common/util.h"

DEFINE_string(ip, "", "TCP ip of this server");
DEFINE_int32(port, 8002, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");
DEFINE_int32(internal_port, -1, "Only allow builtin services at this port");
DEFINE_int32(ping_interval_ms, 1000, "The interval ms to ping meta server");
DEFINE_int32(statistics_interval_ms, 10 * 1000, "The interval ms to statistics table size");
DEFINE_int32(request_timeout_ms, 3000, "Nodeserver request timeout in ms");
DEFINE_int32(expired_interval_ms, 3600 * 1000, "Nodeserver check expired in ms");
DEFINE_string(meta_group, "MetaServer", "MetaServer gorup name");
DEFINE_string(meta_conf, "", "Configuration of MetaServer");
DEFINE_string(eth, "eth1", "Init eth1 get localhost ip");
DECLARE_bool(open_statistic);

int main(int argc, char* argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  butil::AtExitManager exit_manager;
  brpc::Server server;

  // Start NodeServer
  noah::node::NodeServerOptions node_options;
  node_options.meta_group = FLAGS_meta_group;
  node_options.meta_conf = FLAGS_meta_conf;
  node_options.ping_interval_ms = FLAGS_ping_interval_ms;
  node_options.statistics_interval_ms = FLAGS_statistics_interval_ms;
  node_options.expired_interval_ms = FLAGS_expired_interval_ms;
  node_options.node_timeout_ms = FLAGS_request_timeout_ms;

  std::string ipStr;
  if (FLAGS_ip.empty()) {
    node_options.ip = butil::my_ip_cstr();
    if (!noah::common::GetIpAddress(&ipStr, FLAGS_eth) || ipStr.empty()) {
      LOG(ERROR) << "Get ip address failed!";
      return -1;
    }

    node_options.ip = ipStr;
  } else {
    node_options.ip = FLAGS_ip;
  }

  node_options.port = FLAGS_port;
  noah::node::NodeServer node_server(node_options);
  if (0 != node_server.Start()) {
    LOG(ERROR) << "Failed to start NodeServer";
    return -1;
  }

  // Instance of your service.
  noah::node::NodeServiceImpl cache_service_impl(&node_server);

  // Add the service into server. Notice the second parameter, because the
  // service is put on stack, we don't want server to delete it, otherwise
  // use brpc::SERVER_OWNS_SERVICE.
  if (server.AddService(&cache_service_impl,
        brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Failed to add service";
    return -1;
  }

  // Start the server.
  brpc::ServerOptions options; options.idle_timeout_sec = FLAGS_idle_timeout_s;
  options.max_concurrency = FLAGS_max_concurrency;
  options.internal_port = FLAGS_internal_port;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Failed to start NodeService";
    return -1;
  }

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  server.RunUntilAskedToQuit();
  node_server.ShutDown();
  return 0;
}
