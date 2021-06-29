// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_COMMON_CHANNELS_H_
#define NOAH_COMMON_CHANNELS_H_

#include <memory>

#include <string>
#include <unordered_map>
#include <vector>

#include "butil/synchronization/rw_mutex.h"

#include "common/entity.h"

namespace brpc {
class Channel;
class ParallelChannel;
}

typedef std::shared_ptr<brpc::Channel> ChannelPtr;
typedef std::shared_ptr<brpc::ParallelChannel> ParallelChannelPtr;
typedef std::unordered_map<std::string, ChannelPtr> ChannelMap;
typedef std::unordered_map<std::string, ParallelChannelPtr> ParallelChannelMap;

namespace noah {
namespace common {
struct ChannelOptions {
  ChannelOptions() : timeout_ms(0), max_retry(0) {
    protocol = "baidu_std";
  }

  int32_t       timeout_ms;
  int32_t       max_retry;
  std::string   protocol;
  std::string   connection_type;
  std::string   load_balancer;
};

class Channels {
 public:
  explicit Channels(const ChannelOptions& options);
  ~Channels();

  ChannelPtr GetChannelByNode(const Node& node);
  void GetChannelsByNodes(const std::vector<Node>& nodes, std::vector<ChannelPtr>* channels);
  ParallelChannelPtr GetParallelChannelByNodes(const std::vector<Node>& nodes);

 private:
  std::string NodesToString(const std::vector<Node>& nodes) const;

  ChannelOptions        options_;
  butil::RWMutex        lock_;
  ChannelMap            channels_;
  ParallelChannelMap    parallel_channels_;
};

}  // namespace common
}  // namespace noah
#endif
