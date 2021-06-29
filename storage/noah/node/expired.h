// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_EXPIRED_H_
#define NOAH_NODE_EXPIRED_H_

#include <string>
#include <map>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <google/protobuf/map.h>

#include "butil/synchronization/lock.h"

namespace noah {
namespace node {
typedef std::vector<std::string> ArrayString;

class Expired {
 public:
  typedef std::unordered_set<std::string> SetString;
  typedef std::map<int32_t, SetString> MapExpired;
  typedef std::unordered_map<std::string, int32_t> MapString;
  typedef ::google::protobuf::Map<std::string, std::string> GoogleMap;

  Expired();
  ~Expired();

  void SetExpired(const ArrayString& keys, int32_t ttl);
  void SetExpired(const GoogleMap& key_values, int32_t ttl);
  void MigrateExpired(const GoogleMap& key_values);
  void CheckExpired(ArrayString* keys);

 private:
  void SetExpired(const std::string& key, int32_t expired);

  butil::Lock   lock_;
  MapExpired    expired_;
  MapString     keys_info_;
};
}  // namespace node
}  // namespace noah

#endif
