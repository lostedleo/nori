// Author: Zhenwei Zhu losted.leo@gmail.com

#include "tools/noah_client.h"

#include "client/cluster.h"
#include "meta/meta.pb.h"

namespace noah {
namespace client {
NoahClient::NoahClient(Cluster* cluster) : cluster_(cluster) {
  assert(!cluster_);
}

NoahClient::~NoahClient() {
}


bool NoahClient::FindTable(const std::string& table_name, noah::common::Table* info) {
  if (!cluster_->ExistTable(table_name)) {
    return false;
  }
  butil::ReaderAutoLock lock_r(&cluster_->meta_lock_);
  auto iter = cluster_->meta_info_.table_info.find(table_name);
  if (iter == cluster_->meta_info_.table_info.end()) {
    return false;
  }
  if (info) {
    *info = iter->second;
  }
  return true;
}

bool NoahClient::Set(const std::string& table_name, const std::string& key,
                     const std::string& value, int32_t ttl) {
  KeyValueMap key_values;
  key_values[key] = value;
  noah::common::Status status = cluster_->Set(table_name, key_values, ttl);
  if (status != noah::common::kOk) {
    return false;
  }
  return true;
}

bool NoahClient::Get(const std::string& table_name, const std::string& key,
                     std::string* value) {
  StringVector keys;
  keys.push_back(key);
  KeyValueMap key_values;
  noah::common::Status status = cluster_->Get(table_name, keys, &key_values);
  if (status != noah::common::kOk) {
    return false;
  }
  *value = key_values[key];
  return true;
}

bool NoahClient::Del(const std::string& table_name, const std::string& key) {
  StringVector keys;
  keys.push_back(key);
  noah::common::Status status = cluster_->Delete(table_name, keys);
  if (status != noah::common::kOk) {
    return false;
  }
  return true;
}

bool NoahClient::Info(noah::common::MetaInfo** info) {
  StringVector tables;
  if (!cluster_->UpdateTableInfo(tables, true)) {
    return false;
  }
  *info = &cluster_->meta_info_;
  return true;
}

bool NoahClient::GetMetaNodes(noah::meta::MetaNodes* meta_nodes) {
  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::LISTMETA);
  request.mutable_version()->set_epoch(cluster_->meta_info_.version.epoch);
  request.mutable_version()->set_version(cluster_->meta_info_.version.version);
  if (!cluster_->meta_client_.MetaCmd(&request, &response, 1000)) {
    return false;
  }
  if (noah::meta::Status::OK != response.status()) {
    return false;
  }
  *meta_nodes = response.list_meta().nodes();

  return true;
}

bool NoahClient::DropTable(const std::string& table_name) {
  StringVector tables;
  tables.push_back(table_name);
  if (noah::common::Status::kOk != cluster_->DropTable(tables)) {
    return false;
  }
  return true;
}

Iterator* NoahClient::NewIterator(const std::string& table_name) {
  return cluster_->NewIterator(table_name);
}
}  // namespace client
}  // namespace noah

