// auth Zhenwei Zhu losted.leo@gmail.com

#include "meta/meta_service.h"
#include "meta/meta_server.h"

namespace noah {
namespace meta {
MetaServiceImpl::MetaServiceImpl(MetaServer* server) : meta_server_(server) {
}

MetaServiceImpl::~MetaServiceImpl() {
}

void MetaServiceImpl::MetaCmd(::google::protobuf::RpcController* controller,
                              const MetaCmdRequest* request,
                              MetaCmdResponse* response,
                              ::google::protobuf::Closure* done) {
  meta_server_->Apply(request, response, done);
}
}  // namespace meta
}  // namespace noah
