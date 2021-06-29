// Author: Zhenwei Zhu losted.leo@gmail.com

#include "common/channels.h"

#include "brpc/channel.h"
#include "brpc/parallel_channel.h"

namespace noah {
namespace common {
Channels::Channels(const ChannelOptions& options) : options_(options) {
}

Channels::~Channels() {
}

ChannelPtr Channels::GetChannelByNode(const Node& node) {
  std::string node_str = node.ToString();
  {
    butil::ReaderAutoLock lock_r(&lock_);
    auto iter = channels_.find(node_str);
    if (iter != channels_.end()) {
      return iter->second;
    }
  }
  ChannelPtr channel(new brpc::Channel);
  brpc::ChannelOptions options;
  options.protocol = options_.protocol;
  options.connection_type = options_.connection_type;
  options.max_retry = options_.max_retry;
  if (0 != channel->Init(node.ToString().c_str(), options_.load_balancer.c_str(),
                         &options)) {
    LOG(ERROR) << "Failed to initialize channel to " << node.ToString();
    channel.reset();
    return channel;
  }

  butil::WriterAutoLock lock_w(&lock_);
  auto iter = channels_.find(node_str);
  if (iter != channels_.end()) {
    return iter->second;
  }
  channels_[node_str] = channel;
  return channel;
}

void Channels::GetChannelsByNodes(const std::vector<Node>& nodes,
                                 std::vector<ChannelPtr>* channels) {
  int index = 0;
  channels->resize(nodes.size());
  for (const auto& node : nodes) {
    (*channels)[index++] = GetChannelByNode(node);
  }
  return;
}

ParallelChannelPtr Channels::GetParallelChannelByNodes(const std::vector<Node>& nodes) {
  std::string nodes_str = NodesToString(nodes);
  {
    butil::ReaderAutoLock lock_r(&lock_);
    auto iter = parallel_channels_.find(nodes_str);
    if (iter != parallel_channels_.end()) {
      return iter->second;
    }
  }
  ParallelChannelPtr parallel_channel(new brpc::ParallelChannel);
  brpc::ParallelChannelOptions options;
  options.timeout_ms = options_.timeout_ms;
  if (0 != parallel_channel->Init(&options)) {
    LOG(ERROR) << "Failed to initialize parallel_channel to " << nodes_str;
    parallel_channel.reset();
    return parallel_channel;
  }
  for (const auto& node : nodes) {
    ChannelPtr sub_channel = GetChannelByNode(node);
    if (0 != parallel_channel->AddChannel(sub_channel.get(),
                                          brpc::DOESNT_OWN_CHANNEL, NULL, NULL)) {
      LOG(ERROR) << "Failed to add channel " << node.ToString();
      parallel_channel.reset();
      return parallel_channel;
    }
  }

  butil::ReaderAutoLock lock_r(&lock_);
  parallel_channels_[nodes_str] = parallel_channel;
  return parallel_channel;
}

std::string Channels::NodesToString(const std::vector<Node>& nodes) const {
  std::string result;
  for (size_t i = 0; i < nodes.size(); ++i) {
    result += nodes[i].ToString();
    if (i < nodes.size() - 1) {
      result += "_";
    }
  }
  return result;
}

}  // namespace common
}  // namespace noah

