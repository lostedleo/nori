// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_CACHE_H_
#define NOAH_NODE_CACHE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/map.h>

#include "butil/synchronization/rw_mutex.h"

namespace noah {
namespace node {
class Expired;

typedef std::vector<std::string> ArrayString;
typedef std::unordered_map<std::string, std::string> Map;

struct ListInfo {
  ListInfo() : offset(0), size(0) {
  }

  ListInfo(int32_t o, int32_t s, const std::string& key)
    : offset(o), size(s), offset_key(key) {
  }

  int32_t offset;
  int32_t size;
  std::string offset_key;
};

class Cache {
 public:
  typedef ::google::protobuf::Map<std::string, std::string> GoogleMap;
  typedef ::google::protobuf::RepeatedPtrField<std::string> GoogleRepeated;

  explicit Cache(int64_t capacity = 0);
  ~Cache() {}
  bool Get(const GoogleRepeated& keys, GoogleRepeated* values);
  bool Set(const GoogleMap& key_values, int32_t ttl = -1);
  bool Set(const GoogleMap& key_values, const std::vector<int32_t>& ttl_vec);
  bool Del(const GoogleRepeated& keys);
  bool Del(const ArrayString& keys);
  bool Migrate(const GoogleMap& key_values, bool expired);
  bool List(const ListInfo& info, GoogleMap* key_values, int32_t* total_size,
            bool* end, std::string* last_key);
  bool Clear();

  bool expired() { return expired_; }
  int64_t CheckExpired(int64_t* size);
  inline int32_t Size() { return key_values_.size(); }

 private:
  friend class NodeMigrate;

  bool            expired_;
  butil::RWMutex  lock_;
  Map             key_values_;
};
}  // namespace node
}  // namespace noah

#endif
