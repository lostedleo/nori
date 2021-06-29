// Author: Zhenwei Zhu losted.leo@gmail.com

#include "node/node_service.h"

#include <gflags/gflags.h>

#include "brpc/server.h"

#include "node/node_server.h"
#include "node/node.pb.h"
#include "common/const.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_bool(open_statistic, false, "Statistic node_server data");

namespace noah {
namespace node {
void NodeServiceImpl::CreateTable(google::protobuf::RpcController* controller,
                                  const CreateTableRequest* request,
                                  CreateTableResponse* response,
                                  google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

  node_server_->CreateTable(request, response);
  if (FLAGS_echo_attachment) {
    cntl->response_attachment().append(cntl->request_attachment());
  }
}

void NodeServiceImpl::Get(google::protobuf::RpcController* controller,
                          const GetRequest* request,
                          GetResponse* response,
                          google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);
  node_server_->Get(request, response);
  if (FLAGS_echo_attachment) {
    cntl->response_attachment().append(cntl->request_attachment());
  }
}

void NodeServiceImpl::Set(::google::protobuf::RpcController* controller,
                          const SetRequest* request,
                          SetResponse* response,
                          ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

  node_server_->Set(request, response);
  if (FLAGS_echo_attachment) {
    cntl->response_attachment().append(cntl->request_attachment());
  }
}

void NodeServiceImpl::Del(::google::protobuf::RpcController* controller,
                          const DelRequest* request,
                          DelResponse* response,
                          ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

  node_server_->Del(request, response);
  if (FLAGS_echo_attachment) {
    cntl->response_attachment().append(cntl->request_attachment());
  }
}

void NodeServiceImpl::List(::google::protobuf::RpcController* controller,
                           const ListRequest* request,
                           ListResponse* response,
                           ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);
  node_server_->List(request, response);
  if (FLAGS_echo_attachment) {
    cntl->response_attachment().append(cntl->request_attachment());
  }
}

void NodeServiceImpl::Migrate(::google::protobuf::RpcController* controller,
                              const MigrateRequest* request,
                              MigrateResponse* response,
                              ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

  node_server_->Migrate(request, response);
  if (FLAGS_echo_attachment) {
    cntl->response_attachment().append(cntl->request_attachment());
  }
}
}  // namespace node
}  // namespace noah
