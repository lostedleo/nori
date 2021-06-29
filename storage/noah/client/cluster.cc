// Author: Zhenwei Zhu losted.leo@gmail.com

#include "client/cluster.h"

#include "butil/logging.h"
#include "butil/hash.h"
#include "brpc/controller.h"

#include "meta/meta.pb.h"
#include "node/node.pb.h"

DEFINE_int32(cluster_max_retry, 1, "Node request retry number");

namespace noah {
namespace client {
typedef noah::common::Node            Node;
typedef noah::common::Table           Table;

typedef noah::node::SetRequest        SetRequest;
typedef noah::node::SetResponse       SetResponse;
typedef noah::node::ComboSetRequest   ComboSetRequest;
typedef noah::node::ComboSetResponse  ComboSetResponse;
typedef noah::node::GetRequest        GetRequest;
typedef noah::node::GetResponse       GetResponse;
typedef noah::node::ComboGetRequest   ComboGetRequest;
typedef noah::node::ComboGetResponse  ComboGetResponse;
typedef noah::node::DelRequest        DelRequest;
typedef noah::node::DelResponse       DelResponse;
typedef noah::node::ComboDelRequest   ComboDelRequest;
typedef noah::node::ComboDelResponse  ComboDelResponse;
typedef noah::node::ListRequest       ListRequest;
typedef noah::node::ListResponse      ListResponse;
typedef noah::node::ComboListRequest  ComboListRequest;
typedef noah::node::ComboListResponse ComboListResponse;


Status ConvertNodeStatus(noah::node::Status status) {
  Status ret = Status::kOk;
  switch (status) {
    case noah::node::Status::OK:
      ret = Status::kOk;
      break;
    case noah::node::Status::ERROR:
      ret = Status::kError;
      break;
    case noah::node::Status::EEPOCH:
    case noah::node::Status::NOTMASTER:
      ret = Status::kShouldUpdate;
      break;
    case noah::node::Status::NOTFOUND:
      ret = Status::kNotFound;
      break;
    case noah::node::Status::EXIST:
      ret = Status::kExist;
      break;
    default:
      ret = Status::kError;
  }
  return ret;
}

Status ConvertMetaStatus(noah::meta::Status status) {
  Status ret = Status::kOk;
  switch (status) {
    case noah::meta::Status::OK:
      ret = Status::kOk;
      break;
    case noah::meta::Status::ERROR:
      ret = Status::kError;
      break;
    case noah::meta::Status::EEPOCH:
    case noah::meta::Status::REDIRECT:
      ret = Status::kShouldUpdate;
      break;
    case noah::meta::Status::NOTFOUND:
      ret = Status::kNotFound;
      break;
    case noah::meta::Status::EXIST:
      ret = Status::kExist;
      break;
    default:
      ret = Status::kError;
  }
  return ret;
}

class BatchSetRequest : public brpc::CallMapper {
 public:
  brpc::SubCall Map(int channel_index,
                    const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response) {
    const ComboSetRequest* req = dynamic_cast<const ComboSetRequest*>(request);
    ComboSetResponse* res = dynamic_cast<ComboSetResponse*>(response);
    if (method->name() != "ComboSet" ||
        res == NULL || req == NULL ||
        req->requests_size() <= channel_index) {
      return brpc::SubCall::Bad();
    }
    return brpc::SubCall(noah::node::NodeService::descriptor()->FindMethodByName("Set"),
                         &req->requests(channel_index),
                         res->add_responses(), 0);
  }
};

class BatchGetRequest : public brpc::CallMapper {
 public:
  brpc::SubCall Map(int channel_index,
                    const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response) {
    const ComboGetRequest* req = dynamic_cast<const ComboGetRequest*>(request);
    ComboGetResponse* res = dynamic_cast<ComboGetResponse*>(response);
    if (method->name() != "ComboGet" ||
        res == NULL || req == NULL ||
        req->requests_size() <= channel_index) {
      return brpc::SubCall::Bad();
    }
    return brpc::SubCall(noah::node::NodeService::descriptor()->FindMethodByName("Get"),
                         &req->requests(channel_index),
                         res->add_responses(), 0);
  }
};

class BatchDelRequest : public brpc::CallMapper {
 public:
  brpc::SubCall Map(int channel_index,
                    const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response) {
    const ComboDelRequest* req = dynamic_cast<const ComboDelRequest*>(request);
    ComboDelResponse* res = dynamic_cast<ComboDelResponse*>(response);
    if (method->name() != "ComboDel" ||
        res == NULL || req == NULL ||
        req->requests_size() <= channel_index) {
      return brpc::SubCall::Bad();
    }
    return brpc::SubCall(noah::node::NodeService::descriptor()->FindMethodByName("Del"),
                         &req->requests(channel_index),
                         res->add_responses(), 0);
  }
};

class BatchListRequest : public brpc::CallMapper {
 public:
  brpc::SubCall Map(int channel_index,
                    const google::protobuf::MethodDescriptor* method,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response) {
    const ComboListRequest* req = dynamic_cast<const ComboListRequest*>(request);
    ComboListResponse* res = dynamic_cast<ComboListResponse*>(response);
    if (method->name() != "ComboList" ||
        res == NULL || req == NULL ||
        req->requests_size() <= channel_index) {
      return brpc::SubCall::Bad();
    }
    return brpc::SubCall(noah::node::NodeService::descriptor()->FindMethodByName("List"),
                         &req->requests(channel_index),
                         res->add_responses(), 0);
  }
};

class MergeNothing : public brpc::ResponseMerger {
  Result Merge(google::protobuf::Message* response,
               const google::protobuf::Message* sub_response) {
    return brpc::ResponseMerger::MERGED;
  }
};

struct RequestInfo {
  RequestInfo() { }
  RequestInfo(const VersionInfo& info, noah::common::Table table)
    : version(info), table_info(table) {
  }

  VersionInfo           version;
  noah::common::Table   table_info;
};

butil::AtExitManager Cluster::g_exit_manager_;

Cluster::Cluster(const Options& options)
  : options_(options), meta_client_(options_.meta_group, options_.meta_conf) {
  noah::common::ChannelOptions channel_options;
  channel_options.timeout_ms = options_.request_timeout_ms;
  channel_options.max_retry = FLAGS_cluster_max_retry;
  channel_options.protocol = "baidu_std";
  channels_.reset(new Channels(channel_options));
}

Cluster::Cluster(const std::string& meta_group, const std::string& meta_conf)
  : options_(meta_group, meta_conf),
    meta_client_(options_.meta_group, options_.meta_conf) {
  noah::common::ChannelOptions channel_options;
  channel_options.timeout_ms = options_.request_timeout_ms;
  channel_options.max_retry = FLAGS_cluster_max_retry;
  channel_options.protocol = "baidu_std";
  channels_.reset(new Channels(channel_options));
}

Cluster::~Cluster() {
}

bool Cluster::Init() {
  if (!meta_client_.Init()) {
    return false;
  }
  return true;
}

Status Cluster::CreateTable(const std::string& table_name, int32_t partition_num,
                            int32_t duplicate_num, int64_t capacity /*=0*/) {
  if (ExistTable(table_name)) {
    return Status::kExist;
  }
  noah::node::CreateTableRequest request;
  noah::node::CreateTableResponse response;
  ChannelPtr channel;
  {
    uint32_t hash = butil::Hash(table_name);
    butil::ReaderAutoLock lock_r(&meta_lock_);
    if (meta_info_.nodes.empty()) {
      LOG(ERROR) << "Node servers is empty";
      return Status::kError;
    }
    int32_t node_index = hash % meta_info_.nodes.size();
    Node node = meta_info_.nodes[node_index].node();
    channel = channels_->GetChannelByNode(node);
    request.set_epoch(meta_info_.version.epoch);
    request.set_version(meta_info_.version.version);
    request.set_table_name(table_name);
    request.set_parition_num(partition_num);
    request.set_duplicate_num(duplicate_num);
    request.set_capacity(capacity);
  }
  brpc::Controller cntl;
  cntl.set_timeout_ms(options_.request_timeout_ms);
  google::protobuf::RpcChannel* rpc_channel =
    reinterpret_cast<google::protobuf::RpcChannel*>(channel.get());
  noah::node::NodeService_Stub stub(rpc_channel);
  stub.CreateTable(&cntl, &request, &response, NULL);
  if (cntl.Failed()) {
    LOG(ERROR) << "Failed to request NodeServer error " << cntl.ErrorText();
    return Status::kError;
  }
  if (response.status() == noah::node::Status::OK
      || response.status() == noah::node::Status::EXIST) {
    if (!UpdateWithTable(table_name)) {
      return Status::kError;
    }
  } else {
    return Status::kError;
  }
  return Status::kOk;
}

Status Cluster::Set(const std::string& table_name, const KeyValueMap& key_values,
                    int32_t ttl) {
  Status status;
  int32_t retry_num = options_.retry_num;
  do {
    status = BatchSet(table_name, key_values, ttl);
    if (Status::kOk == status) {
      break;
    } else if (Status::kShouldUpdate == status) {
      UpdateMetaInfo(VersionInfo(meta_info_.version.epoch,
                                 meta_info_.version.version + 1));
    } else if (Status::kNotFound == status) {
      if (!options_.create_table) {
        return status;
      }
      status = CreateTable(table_name, options_.auto_partition_num,
                           options_.auto_duplicate_num, options_.auto_capacity);
      if (Status::kOk == status) {
        continue;
      }
    } else {
      usleep(500 * 1000);
    }
    retry_num--;
  } while (retry_num >= 0);
  return status;
}

Status Cluster::Get(const std::string& table_name, const StringVector& keys,
                    KeyValueMap* key_values) {
  Status status;
  int32_t retry_num = options_.retry_num;
  key_values->reserve(keys.size());
  do {
    status = BatchGet(table_name, keys, key_values);
    if (Status::kOk == status) {
      break;
    } else if (Status::kShouldUpdate == status) {
      UpdateMetaInfo(VersionInfo(meta_info_.version.epoch,
                                 meta_info_.version.version + 1));
    } else {
      usleep(500 * 1000);
    }
    retry_num--;
  } while (retry_num >= 0);
  return status;
}

Status Cluster::Delete(const std::string& table_name, const StringVector& keys) {
  Status status;
  int32_t retry_num = options_.retry_num;
  do {
    status = BatchDelete(table_name, keys);
    if (Status::kOk == status) {
      break;
    } else if (Status::kShouldUpdate == status) {
      UpdateMetaInfo(VersionInfo(meta_info_.version.epoch,
                                 meta_info_.version.version + 1));
    } else {
      usleep(500 * 1000);
    }
    retry_num--;
  } while (retry_num >= 0);
  return status;
}

Iterator* Cluster::NewIterator(const std::string& table_name) {
  return new Iterator(table_name, this);
}

Status Cluster::ListTableInfo(const std::string& table_name,
                              std::vector<int32_t>* offset_info) {
  Status status;
  int32_t retry_num = options_.retry_num;
  do {
    status = InnerListTableInfo(table_name, offset_info);
    if (Status::kOk == status) {
      break;
    } else if (Status::kShouldUpdate == status) {
      UpdateMetaInfo(VersionInfo(meta_info_.version.epoch,
                                 meta_info_.version.version + 1));
    } else if (Status::kNotFound == status) {
      return status;
    } else {
      usleep(500 * 1000);
    }
    retry_num--;
  } while (retry_num >= 0);
  return status;
}

Status Cluster::ListTable(const std::string& table_name,
                          const VectorSplitInfo& split_infos,
                          KeyValueMap* key_values,
                          std::string* last_key /* =NULL */) {
  Status status;
  int32_t retry_num = options_.retry_num;
  do {
    status = InnerListTable(table_name, split_infos, key_values, last_key);
    if (Status::kOk == status) {
      break;
    } else if (Status::kShouldUpdate == status) {
      UpdateMetaInfo(VersionInfo(meta_info_.version.epoch,
                                 meta_info_.version.version + 1));
    } else if (Status::kNotFound == status) {
      return status;
    } else {
      usleep(500 * 1000);
    }
    retry_num--;
  } while (retry_num >= 0);
  return status;
}

Status Cluster::DropTable(const StringVector& tables) {
  if (meta_info_.version.epoch < 0) {
    if (!UpdateMetaInfo(VersionInfo(0, 0))) {
      return Status::kError;
    }
  }

  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::Type::DROPTABLE);
  request.mutable_version()->set_epoch(meta_info_.version.epoch);
  request.mutable_version()->set_version(meta_info_.version.version);
  for (const auto& table : tables) {
    request.mutable_drop_table()->add_names(table);
  }
  if (!meta_client_.MetaCmd(&request, &response, options_.request_timeout_ms)) {
    return Status::kError;
  }

  if (noah::meta::Status::OK == response.status()) {
    butil::WriterAutoLock lock_w(&meta_lock_);
    meta_info_.version = VersionInfo(response.version());
    for (const auto& table : tables) {
      meta_info_.table_info.erase(table);
    }
  } else if (noah::meta::Status::REDIRECT == response.status()) {
    LOG(INFO) << "Meta leader change to " << response.redirect();
    meta_client_.UpdateLeader(response.redirect());
    return DropTable(tables);
  } else {
    LOG(ERROR) << "Request meta info error " << response.DebugString();
    return Status::kError;
  }
  return Status::kOk;
}

Status Cluster::FlushTable(const std::string& table) {
  return Status::kOk;
}

Status Cluster::BatchSet(const std::string& table_name,
                         const KeyValueMap& key_values, int32_t ttl) {
  RequestInfo request_info;
  if (!FindTable(table_name, &request_info)) {
    LOG(ERROR) << "Table not exist " << table_name;
    return Status::kNotFound;
  }
  ComboSetRequest combo_request;
  ComboSetResponse combo_response;
  brpc::ParallelChannel channel;
  brpc::ParallelChannelOptions options;
  options.fail_limit = 1;
  options.timeout_ms = options_.request_timeout_ms;
  channel.Init(&options);

  std::vector<Node> nodes;
  request_info.table_info.GetAllMasters(&nodes);
  int32_t partition_num = request_info.table_info.partition_num();
  std::vector<KeyValueMap> split_result;
  SplitSetRequst(key_values, partition_num, &split_result);
  std::vector<ChannelPtr> sub_channels;
  channels_->GetChannelsByNodes(nodes, &sub_channels);
  for (int i = 0; i < partition_num; ++i) {
    if (split_result[i].empty()) {
      continue;
    }
    SetRequest* sub_request = combo_request.add_requests();
    sub_request->set_epoch(request_info.version.epoch);
    sub_request->set_version(request_info.table_info.version());
    sub_request->set_table_name(table_name);
    sub_request->set_id(i);
    sub_request->set_expired(ttl);
    for (const auto& value : split_result[i]) {
      (*sub_request->mutable_key_values())[value.first] = value.second;
    }
    if (0 != channel.AddChannel(sub_channels[i].get(), brpc::DOESNT_OWN_CHANNEL,
                                new BatchSetRequest, new MergeNothing)) {
      LOG(ERROR) << "Failed to add channel " << nodes[i].ToString();
      return Status::kShouldUpdate;
    }
  }
  brpc::Controller cntl;
  cntl.set_timeout_ms(options_.request_timeout_ms);
  noah::node::NodeService_Stub stub(&channel);
  stub.ComboSet(&cntl, &combo_request, &combo_response, NULL);
  if (cntl.Failed()) {
    LOG(ERROR) << "Failed to request NodeServer error " << cntl.ErrorText();
    if (112 == cntl.ErrorCode()) {
      return Status::kShouldUpdate;
    }
    return Status::kError;
  }
  int32_t index = 0;
  for (const auto& res : combo_response.responses()) {
    if (noah::node::Status::OK != res.status()) {
      LOG(ERROR) << "Request to NodeServer " << nodes[index].ToString() << " error "
                 << res.status();
      return ConvertNodeStatus(res.status());
    }
    index++;
  }

  return Status::kOk;
}

Status Cluster::BatchGet(const std::string& table_name, const StringVector& keys,
                         KeyValueMap* key_values) {
  RequestInfo request_info;
  if (!FindTable(table_name, &request_info)) {
    LOG(ERROR) << "Table not exist " << table_name;
    return Status::kNotFound;
  }
  ComboGetRequest combo_request;
  ComboGetResponse combo_response;
  brpc::ParallelChannel channel;
  brpc::ParallelChannelOptions options;
  options.fail_limit = 1;
  options.timeout_ms = options_.request_timeout_ms;
  channel.Init(&options);

  std::vector<Node> nodes;
  request_info.table_info.GetAllMasters(&nodes);
  int32_t partition_num = request_info.table_info.partition_num();
  std::vector<StringVector> split_keys;
  SplitArrayRquest(keys, partition_num, &split_keys);
  std::vector<ChannelPtr> sub_channels;
  channels_->GetChannelsByNodes(nodes, &sub_channels);
  for (int i = 0; i < partition_num; ++i) {
    if (split_keys[i].empty()) {
      continue;
    }
    GetRequest* sub_request = combo_request.add_requests();
    sub_request->set_epoch(request_info.version.epoch);
    sub_request->set_version(request_info.table_info.version());
    sub_request->set_table_name(table_name);
    sub_request->set_id(i);
    for (const auto& key : split_keys[i]) {
      sub_request->add_keys(key);
    }
    if (0 != channel.AddChannel(sub_channels[i].get(), brpc::DOESNT_OWN_CHANNEL,
                                new BatchGetRequest, new MergeNothing)) {
      LOG(ERROR) << "Failed to add channel " << nodes[i].ToString();
      return Status::kShouldUpdate;
    }
  }

  brpc::Controller cntl;
  cntl.set_timeout_ms(options_.request_timeout_ms);
  noah::node::NodeService_Stub stub(&channel);
  stub.ComboGet(&cntl, &combo_request, &combo_response, NULL);
  if (cntl.Failed()) {
    LOG(ERROR) << "Failed to request NodeServer error " << cntl.ErrorText();
    if (112 == cntl.ErrorCode()) {
      return Status::kShouldUpdate;
    }
    return Status::kError;
  }
  int32_t index = 0;
  for (const auto& res : combo_response.responses()) {
    if (noah::node::Status::OK == res.status()) {
      const auto& req = combo_request.requests(index);
      int32_t size = req.keys_size();
      for (int i = 0; i < size; ++i) {
        (*key_values)[req.keys(i)] = res.values(i);
      }
    } else {
      LOG(ERROR) << "Request to NodeServer " << nodes[index].ToString() << " error "
                 << res.status();
      return ConvertNodeStatus(res.status());
    }
    index++;
  }

  return Status::kOk;
}

Status Cluster::BatchDelete(const std::string& table_name,
                            const StringVector& keys) {
  RequestInfo request_info;
  if (!FindTable(table_name, &request_info)) {
    LOG(ERROR) << "Table not exist " << table_name;
    return Status::kNotFound;
  }
  ComboDelRequest combo_request;
  ComboDelResponse combo_response;
  brpc::ParallelChannel channel;
  brpc::ParallelChannelOptions options;
  options.fail_limit = 1;
  options.timeout_ms = options_.request_timeout_ms;
  channel.Init(&options);

  std::vector<Node> nodes;
  request_info.table_info.GetAllMasters(&nodes);
  int32_t partition_num = request_info.table_info.partition_num();
  std::vector<StringVector> split_keys;
  SplitArrayRquest(keys, partition_num, &split_keys);
  std::vector<ChannelPtr> sub_channels;
  channels_->GetChannelsByNodes(nodes, &sub_channels);
  for (int i = 0; i < partition_num; ++i) {
    if (split_keys[i].empty()) {
      continue;
    }
    DelRequest* sub_request = combo_request.add_requests();
    sub_request->set_epoch(request_info.version.epoch);
    sub_request->set_version(request_info.table_info.version());
    sub_request->set_table_name(table_name);
    sub_request->set_id(i);
    for (const auto& key : split_keys[i]) {
      sub_request->add_keys(key);
    }
    if (0 != channel.AddChannel(sub_channels[i].get(), brpc::DOESNT_OWN_CHANNEL,
                               new BatchDelRequest, new MergeNothing)) {
      LOG(ERROR) << "Failed to add channel " << nodes[i].ToString();
      return Status::kShouldUpdate;
    }
  }

  brpc::Controller cntl;
  cntl.set_timeout_ms(options_.request_timeout_ms);
  noah::node::NodeService_Stub stub(&channel);
  stub.ComboDel(&cntl, &combo_request, &combo_response, NULL);
  if (cntl.Failed()) {
    LOG(ERROR) << "Failed to request NodeServer error " << cntl.ErrorText();
    if (112 == cntl.ErrorCode()) {
      return Status::kShouldUpdate;
    }
    return Status::kError;
  }
  int32_t index = 0;
  for (const auto& res : combo_response.responses()) {
    if (noah::node::Status::OK != res.status()) {
      LOG(ERROR) << "Request to NodeServer " << nodes[index].ToString() << " error "
                 << res.status();
      return ConvertNodeStatus(res.status());
    }
    index++;
  }

  return Status::kOk;
}

Status Cluster::InnerListTableInfo(const std::string& table_name,
                                   std::vector<int32_t>* offset_info) {
  offset_info->clear();
  RequestInfo request_info;
  if (!FindTable(table_name, &request_info)) {
    LOG(ERROR) << "Table not exist " << table_name;
    return Status::kNotFound;
  }
  ComboListRequest combo_request;
  ComboListResponse combo_response;
  brpc::ParallelChannel channel;
  brpc::ParallelChannelOptions options;
  options.fail_limit = 1;
  options.timeout_ms = options_.request_timeout_ms;
  channel.Init(&options);

  std::vector<Node> nodes;
  request_info.table_info.GetAllMasters(&nodes);
  int32_t partition_num = request_info.table_info.partition_num();
  std::vector<ChannelPtr> sub_channels;
  channels_->GetChannelsByNodes(nodes, &sub_channels);
  for (int i = 0; i < partition_num; ++i) {
    ListRequest* sub_request = combo_request.add_requests();
    sub_request->set_epoch(request_info.version.epoch);
    sub_request->set_version(request_info.table_info.version());
    sub_request->set_table_name(table_name);
    sub_request->set_id(i);
    sub_request->set_offset(0);
    sub_request->set_size(0);
    if (0 != channel.AddChannel(sub_channels[i].get(), brpc::DOESNT_OWN_CHANNEL,
                                new BatchListRequest, new MergeNothing)) {
      LOG(ERROR) << "Failed to add channel " << nodes[i].ToString();
      return Status::kShouldUpdate;
    }
  }
  brpc::Controller cntl;
  cntl.set_timeout_ms(options_.request_timeout_ms);
  noah::node::NodeService_Stub stub(&channel);
  stub.ComboList(&cntl, &combo_request, &combo_response, NULL);
  if (cntl.Failed()) {
    LOG(ERROR) << "Failed to request NodeServer error " << cntl.ErrorText();
    if (112 == cntl.ErrorCode()) {
      return Status::kShouldUpdate;
    }
    return Status::kError;
  }
  int32_t offset = 0;
  int32_t index = 0;
  for (const auto& res : combo_response.responses()) {
    if (noah::node::Status::OK != res.status()) {
      LOG(ERROR) << "Request to NodeServer " << nodes[index].ToString() << " error "
                 << res.status();
      return ConvertNodeStatus(res.status());
    }
    index++;

    offset += res.total_size();
    offset_info->push_back(offset);
  }

  return Status::kOk;
}

Status Cluster::InnerListTable(const std::string& table_name,
                               const VectorSplitInfo& split_infos,
                               KeyValueMap* key_values,
                               std::string* last_key/* =NULL */) {
  key_values->clear();
  RequestInfo request_info;
  if (!FindTable(table_name, &request_info)) {
    LOG(ERROR) << "Table not exist " << table_name;
    return Status::kNotFound;
  }
  ComboListRequest combo_request;
  ComboListResponse combo_response;
  brpc::ParallelChannel channel;
  brpc::ParallelChannelOptions options;
  options.fail_limit = 1;
  options.timeout_ms = options_.request_timeout_ms;
  channel.Init(&options);

  std::vector<Node> nodes;
  request_info.table_info.GetAllMasters(&nodes);
  int32_t partition_num = request_info.table_info.partition_num();
  std::vector<ChannelPtr> sub_channels;
  channels_->GetChannelsByNodes(nodes, &sub_channels);
  for (const auto& value : split_infos) {
    if (value.id >= partition_num) {
      LOG(ERROR) << "Partition Id is invalid id " << value.id
                 << " partition_num " << partition_num;
      return Status::kError;
    }
    ListRequest* sub_request = combo_request.add_requests();
    sub_request->set_epoch(request_info.version.epoch);
    sub_request->set_version(request_info.table_info.version());
    sub_request->set_table_name(table_name);
    sub_request->set_id(value.id);
    sub_request->set_offset(value.offset);
    sub_request->set_size(value.size);
    if (!value.last_key.empty()) {
      sub_request->set_offset_key(value.last_key);
    }
    if (0 != channel.AddChannel(sub_channels[value.id].get(), brpc::DOESNT_OWN_CHANNEL,
                                new BatchListRequest, new MergeNothing)) {
      LOG(ERROR) << "Failed to add channel " << nodes[value.id].ToString();
      return Status::kShouldUpdate;
    }
  }
  brpc::Controller cntl;
  cntl.set_timeout_ms(options_.request_timeout_ms);
  noah::node::NodeService_Stub stub(&channel);
  stub.ComboList(&cntl, &combo_request, &combo_response, NULL);
  if (cntl.Failed()) {
    LOG(ERROR) << "Failed to request NodeServer error " << cntl.ErrorText();
    if (112 == cntl.ErrorCode()) {
      return Status::kShouldUpdate;
    }
    return Status::kError;
  }
  int32_t index = 0;
  for (const auto& res : combo_response.responses()) {
    if (noah::node::Status::OK != res.status()) {
      LOG(ERROR) << "Request to NodeServer " << nodes[index].ToString() << " error "
                 << res.status();
      return ConvertNodeStatus(res.status());
    }
    index++;

    for (const auto& value : res.key_values()) {
      (*key_values)[value.first] = value.second;
    }
    if (!res.end() && last_key) {
      *last_key = res.key();
    }
  }

  return Status::kOk;
}

bool Cluster::UpdateMetaInfo(const VersionInfo& info) {
  std::vector<std::string> tables;
  {
    butil::ReaderAutoLock lock_r(&meta_lock_);
    if (info.epoch <= meta_info_.version.epoch
        && info.version <= meta_info_.version.version) {
      LOG(INFO) << "Meta info is updated current version " << meta_info_.version.DebugDump()
        << " update version " << info.DebugDump();
      return true;
    }
    for (const auto& value : meta_info_.table_info) {
      tables.push_back(value.first);
    }
  }

  return UpdateTableInfo(tables);
}

bool Cluster::UpdateWithTable(const std::string& table_name) {
  std::vector<std::string> tables;
  {
    butil::ReaderAutoLock lock_r(&meta_lock_);
    for (const auto& value : meta_info_.table_info) {
      tables.push_back(value.first);
    }
    tables.push_back(table_name);
  }
  return UpdateTableInfo(tables);
}

bool Cluster::UpdateTableInfo(const std::vector<std::string>& tables, bool all /* false */) {
  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::Type::PULL);
  request.mutable_version()->set_epoch(meta_info_.version.epoch);
  request.mutable_version()->set_version(meta_info_.version.version);
  if (!all) {
    for (const auto& table : tables) {
      request.mutable_pull()->add_names(table);
    }
  } else {
    request.mutable_pull()->set_all(true);
  }
  if (!meta_client_.MetaCmd(&request, &response, options_.request_timeout_ms)) {
    return false;
  }

  if (noah::meta::Status::OK == response.status()) {
    butil::WriterAutoLock lock_w(&meta_lock_);
    meta_info_.version = VersionInfo(response.version());
    meta_info_.table_info.clear();
    for (const auto& table : response.pull().info()) {
      meta_info_.table_info[table.name()] = noah::common::Table(table);
    }

    meta_info_.nodes.clear();
    for (const auto& value : response.pull().node_status()) {
      meta_info_.nodes.push_back(noah::common::NodeStatus(value));
    }
  } else if (noah::meta::Status::REDIRECT == response.status()) {
    LOG(INFO) << "Meta leader change to " << response.redirect();
    meta_client_.UpdateLeader(response.redirect());
    return UpdateTableInfo(tables, all);
  } else {
    LOG(ERROR) << "Update meta info error " << response.DebugString();
    return false;
  }
  return true;
}

bool Cluster::ExistTable(const std::string& table_name, bool updated /* = false */) {
  {
    butil::ReaderAutoLock lock_r(&meta_lock_);
    auto iter = meta_info_.table_info.find(table_name);
    if (iter == meta_info_.table_info.end()) {
      if (updated) {
        return false;
      }
    } else {
      return true;
    }
  }
  if (!UpdateWithTable(table_name)) {
    return false;
  }
  return ExistTable(table_name, true);
}

bool Cluster::FindTable(const std::string& table_name, RequestInfo* info) {
  if (!ExistTable(table_name)) {
    return false;
  }
  butil::ReaderAutoLock lock_r(&meta_lock_);
  auto iter = meta_info_.table_info.find(table_name);
  if (iter == meta_info_.table_info.end()) {
    return false;
  }
  if (info) {
    info->version = meta_info_.version;
    info->table_info = iter->second;
  }
  return true;
}

void Cluster::SplitArrayRquest(const StringVector& keys, int32_t partition_num,
                               std::vector<StringVector>* split_keys) {
  split_keys->resize(partition_num);
  if (1 == partition_num) {
    (*split_keys)[0] = keys;
    return;
  }

  uint32_t hash = 0;
  for (const auto& key : keys) {
    hash = butil::Hash(key);
    (*split_keys)[hash % partition_num].push_back(key);
  }
}

void Cluster::SplitSetRequst(const KeyValueMap& key_values, int32_t partition_num,
                             std::vector<KeyValueMap>* split_result) {
  split_result->resize(partition_num);
  if (1 == partition_num) {
    (*split_result)[0] = key_values;
    return;
  }

  uint32_t hash = 0;
  for (const auto& value : key_values) {
    hash = butil::Hash(value.first);
    (*split_result)[hash % partition_num][value.first] = value.second;
  }
}

}  // namespace client
}  // namespace noah
