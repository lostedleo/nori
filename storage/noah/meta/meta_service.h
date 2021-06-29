// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_META_SERVICE_H_
#define NOAH_META_SERVICE_H_

#include "meta/meta.pb.h"

namespace noah {
namespace meta {

class MetaServer;

class MetaServiceImpl : public MetaService {
 public:
  explicit MetaServiceImpl(MetaServer* server);
  ~MetaServiceImpl();

  void MetaCmd(::google::protobuf::RpcController* controller,
               const MetaCmdRequest* request,
               MetaCmdResponse* response,
               ::google::protobuf::Closure* done) override;

 private:
  MetaServer* meta_server_;
};
}  // namespace meta
}  // namespace noah

#endif
