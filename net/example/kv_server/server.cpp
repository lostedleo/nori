// Copyright (c) 2014 Baidu, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0 //
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <butil/memory/scoped_ptr.h>

#include "cache.pb.h"
#include "cache.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 8002, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");
DEFINE_int32(internal_port, -1, "Only allow builtin services at this port");

namespace push {
// Your implementation of CacheService
class CacheServiceImpl : public CacheService {
 public:
  CacheServiceImpl(Cache* cache) : cache_(cache) {
    assert(!cache_);
  }
  ~CacheServiceImpl() {};
  void Get(google::protobuf::RpcController* controller,
           const GetRequest* request,
           GetResponse* response,
           google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl =
      static_cast<brpc::Controller*>(controller);

    response->set_status(Status::OK);
    auto values = response->mutable_values();
    cache_->Get(request->keys(), values);
    if (FLAGS_echo_attachment) {
      cntl->response_attachment().append(cntl->request_attachment());
    }
  }

  void Set(::google::protobuf::RpcController* controller,
           const SetRequest* request,
           SetResponse* response,
           ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl =
      static_cast<brpc::Controller*>(controller);

    cache_->Set(request->key_values());
    response->set_status(Status::OK);
    if (FLAGS_echo_attachment) {
      cntl->response_attachment().append(cntl->request_attachment());
    }
  }

 private:
  Cache*  cache_;
};
}  // namespace push

DEFINE_bool(h, false, "print help information");

int main(int argc, char* argv[]) {
  std::string help_str = "dummy help infomation";
  google::SetUsageMessage(help_str);

  // Parse gflags. We recommend you to use gflags as well.
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_h) {
    fprintf(stderr, "%s\n%s\n%s", help_str.c_str(), help_str.c_str(), help_str.c_str());
    return 0;
  }

  // Generally you only need one Server.
  brpc::Server server;

  scoped_ptr<push::Cache> cache(new push::Cache());
  // Instance of your service.
  push::CacheServiceImpl cache_service_impl(cache.get());

  // Add the service into server. Notice the second parameter, because the
  // service is put on stack, we don't want server to delete it, otherwise
  // use brpc::SERVER_OWNS_SERVICE.
  if (server.AddService(&cache_service_impl,
        brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add service";
    return -1;
  }

  // Start the server.
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  options.max_concurrency = FLAGS_max_concurrency;
  options.internal_port = FLAGS_internal_port;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Fail to start CacheServer";
    return -1;
  }

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  server.RunUntilAskedToQuit();
  return 0;
}
