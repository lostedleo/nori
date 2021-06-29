// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_COMMON_META_CLIENT_H_
#define NOAH_COMMON_META_CLIENT_H_

#include <string>
#include <memory>

#include <unordered_map>

#include "butil/synchronization/lock.h"

namespace brpc {
class Channel;
}

namespace noah {
namespace meta {
class MetaCmdRequest;
class MetaCmdResponse;
}
}

namespace noah {
namespace common {
typedef std::shared_ptr<brpc::Channel> ChannelPtr;

class MetaClient {
 public:
  typedef noah::meta::MetaCmdRequest MetaCmdRequest;
  typedef noah::meta::MetaCmdResponse MetaCmdResponse;

  MetaClient(const std::string& group, const std::string& conf);
  ~MetaClient();

  bool Init();
  ChannelPtr GetLeaderChannel(int64_t timeout_ms);
  void UpdateLeader(const std::string& leader);
  bool MetaCmd(const MetaCmdRequest* request, MetaCmdResponse* response,
               int32_t timeout_ms);

 private:
  std::string     group_;
  std::string     conf_;
  butil::Lock     lock_;
  std::string     leader_str_;
  ChannelPtr      leader_channel_;
};
}  // namespace common
}  // namespace noah

#endif
