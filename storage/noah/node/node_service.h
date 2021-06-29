// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_SERVICE_H_
#define NOAH_NODE_SERVICE_H_

#include <assert.h>

#include "butil/logging.h"
#include "node/node.pb.h"

namespace noah {
namespace node {
class NodeServer;

// Implementation of NodeService
class NodeServiceImpl : public NodeService {
 public:
  explicit NodeServiceImpl(NodeServer* node_server) : node_server_(node_server) {
    assert(!node_server_);
  }
  virtual ~NodeServiceImpl() {}

  void CreateTable(google::protobuf::RpcController* controller,
                   const CreateTableRequest* request,
                   CreateTableResponse* response,
                   google::protobuf::Closure* done) override;

  void Get(google::protobuf::RpcController* controller,
           const GetRequest* request,
           GetResponse* response,
           google::protobuf::Closure* done) override;

  void Set(::google::protobuf::RpcController* controller,
           const SetRequest* request,
           SetResponse* response,
           ::google::protobuf::Closure* done) override;

  void Del(::google::protobuf::RpcController* controller,
           const DelRequest* request,
           DelResponse* response,
           ::google::protobuf::Closure* done) override;

  void List(::google::protobuf::RpcController* controller,
           const ListRequest* request,
           ListResponse* response,
           ::google::protobuf::Closure* done) override;

  void Migrate(::google::protobuf::RpcController* controller,
           const MigrateRequest* request,
           MigrateResponse* response,
           ::google::protobuf::Closure* done) override;

 private:
  NodeServer*  node_server_;
};
}  // namespace node
}  // namespace noah
#endif
