// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_TEST_CLIENT_H_
#define NOAH_TEST_CLIENT_H_

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "test_client/options.h"
#include "common/entity.h"

namespace noah {
namespace test_client {
class Cluster;
class Iterator;

typedef noah::common::Status Status;

class RawClient {
 public :
  explicit RawClient(const Options& options);
  RawClient(const std::string& group, const std::string& conf);

  virtual ~RawClient();

  bool Init();
  Status Set(const std::string& table_name, const KeyValueMap& key_values,
             int32_t ttl = -1);
  Status Get(const std::string& table_name, const StringVector& keys,
             KeyValueMap* key_values);
  Status Delete(const std::string& table_name, const StringVector& keys);
  Iterator* NewIterator(const std::string& table_name);

 private :
  std::unique_ptr<Cluster> cluster_;
};

class Client {
 public :
  Client(const Options& options, const std::string& table_name)
      : raw_client_(options),
        table_(table_name) {
  }
  Client(const std::string& group, const std::string& conf, const std::string& table_name)
      : raw_client_(group, conf),
        table_(table_name) {
  }
  virtual ~Client() {}

  bool Init() {
    return raw_client_.Init();
  }

  Status Set(const KeyValueMap& key_values, int32_t ttl = -1) {
    return raw_client_.Set(table_, key_values, ttl);
  }
  Status Get(const StringVector& keys, KeyValueMap* key_values) {
    return raw_client_.Get(table_, keys, key_values);
  }
  Status Delete(const StringVector& keys) {
    return raw_client_.Delete(table_, keys);
  }
  Iterator* NewIterator() {
    return raw_client_.NewIterator(table_);
  }

 private :
  RawClient raw_client_;
  const std::string table_;
};

}  // namespace client
}  // namespace noah
#endif
