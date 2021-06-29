#include "client/client.h"
#include "client/cluster.h"

namespace noah {
namespace client {

RawClient::RawClient(const Options& options)
  : cluster_(new Cluster(options)) {
}

RawClient::RawClient(const std::string& group, const std::string& conf)
  : cluster_(new Cluster(group, conf)) {
}

RawClient::~RawClient() {
}

bool RawClient::Init() {
  return cluster_->Init();
}

Status RawClient::Set(const std::string& table_name, const KeyValueMap& key_values,
                      int32_t ttl) {
  return cluster_->Set(table_name, key_values, ttl);
}

Status RawClient::Get(const std::string& table_name, const StringVector& keys,
                      KeyValueMap* key_values) {
  return cluster_->Get(table_name, keys, key_values);
}

Status RawClient::Delete(const std::string& table_name,
                         const StringVector& keys) {
  return cluster_->Delete(table_name, keys);
}

Iterator* RawClient::NewIterator(const std::string& table_name) {
  return cluster_->NewIterator(table_name);
}

}  // namespace client
}  // namespace noah
