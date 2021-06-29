// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_META_CLOSURE_H_
#define NOAH_META_CLOSURE_H_

#include <braft/raft.h>

namespace noah {
namespace meta {
class MetaServer;
class MetaCmdRequest;
class MetaCmdResponse;

class MetaClosure : public braft::Closure {
 public:
  MetaClosure(MetaServer* server,
              const MetaCmdRequest* request,
              MetaCmdResponse* response,
              ::google::protobuf::Closure* done)
    : meta_server_(server)
    , request_(request)
    , response_(response)
    , done_(done) {
    }

  void Run() override;
  const MetaCmdRequest* request() const { return request_; }
  MetaCmdResponse* response() const { return response_; }

 private:
  MetaServer*                 meta_server_;
  const MetaCmdRequest*       request_;
  MetaCmdResponse*            response_;
  google::protobuf::Closure*  done_;
};
}  // namespace meta
}  // namespace noah
#endif
