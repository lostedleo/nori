// Author: Zhenwei Zhu losted.leo@gmail.com

#include "common/meta_client.h"

#include "braft/route_table.h"
#include "brpc/channel.h"
#include "brpc/controller.h"

#include "meta/meta.pb.h"

namespace noah {
namespace common {
MetaClient::MetaClient(const std::string& group, const std::string& conf)
  : group_(group), conf_(conf) {
}

MetaClient::~MetaClient() {
}

bool MetaClient::Init() {
  if (0 != braft::rtb::update_configuration(group_, conf_)) {
    LOG(ERROR) << "Failed to register configuration " << conf_ << " of group "
               << group_;
    return false;
  }
  return true;
}

ChannelPtr MetaClient::GetLeaderChannel(int64_t timeout_ms) {
  braft::PeerId leader;
  // Select leader of the target group from RouteTable
  if (0 != braft::rtb::select_leader(group_, &leader)) {
    butil::Status status = braft::rtb::refresh_leader(group_, timeout_ms);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to refresh_leader: " << status;
      return leader_channel_;
    }
    return GetLeaderChannel(timeout_ms);
  }

  butil::AutoLock auto_lock(lock_);
  std::string leader_str = leader.to_string();
  if (leader_str_ == leader_str) {
    return leader_channel_;
  }
  leader_channel_.reset(new brpc::Channel());
  brpc::ChannelOptions channel_options;
  channel_options.connect_timeout_ms = timeout_ms;
  if (0 != leader_channel_->Init(leader.addr, &channel_options)) {
    LOG(ERROR) << "Failed to init channel to " << leader;
    leader_channel_.reset();
    return leader_channel_;
  }
  leader_str_ = leader_str;
  return leader_channel_;
}

void MetaClient::UpdateLeader(const std::string& leader) {
  braft::rtb::update_leader(group_, leader);
}

bool MetaClient::MetaCmd(const MetaCmdRequest* request, MetaCmdResponse* response,
                         int32_t timeout_ms) {
  ChannelPtr channel = GetLeaderChannel(timeout_ms);
  if (!channel.get()) {
    LOG(ERROR) << "Meta leader is invalid";
    return false;
  }
  google::protobuf::RpcChannel* rpc_channel =
    reinterpret_cast<google::protobuf::RpcChannel*>(channel.get());
  noah::meta::MetaService_Stub stub(rpc_channel);
  brpc::Controller cntl;
  cntl.set_timeout_ms(timeout_ms);
  stub.MetaCmd(&cntl, request, response, NULL);
  if (cntl.Failed()) {
    LOG(WARNING) << "Failed to send to meta leader error: " << cntl.ErrorText()
                 << " request " << request->DebugString();
    // Clear leadership since this RPC failed.
    braft::rtb::update_leader(group_, braft::PeerId());
    return false;
  }
  return true;
}

}  // namespace common
}  // namespace noah

