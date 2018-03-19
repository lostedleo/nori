#ifndef CACHE_H_
#define CACHE_H_

#include <unordered_map>
#include <string>

#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>

#include <butil/synchronization/lock.h>

namespace push {
class Cache {
 public:
  typedef std::unordered_map<std::string, std::string> Map;
  typedef ::google::protobuf::Map<std::string, std::string> GoogleMap;
  typedef ::google::protobuf::RepeatedPtrField< ::std::string> GoogleRepeated;

  Cache() {}
  ~Cache() {}
  bool Get(const GoogleRepeated& keys, GoogleRepeated* values) const;
  bool Set(const GoogleMap& key_values);

 private:
  mutable butil::Lock   lock_;
  Map                   key_values_;
};
}  // namespace push

#endif
