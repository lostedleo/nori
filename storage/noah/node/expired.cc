// Author: Zhenwei Zhu losted.leo@gmail.com

#include "node/expired.h"

#include "butil/time.h"

#include "node/node.pb.h"

namespace noah {
namespace node {
Expired::Expired() {
}

Expired::~Expired() {
}

void Expired::SetExpired(const ArrayString& keys, int32_t ttl) {
  int32_t current_time = butil::gettimeofday_s();
  int32_t expired_time = current_time + ttl;
  SetString* set_info = NULL;

  butil::AutoLock auto_lock(lock_);
  auto iter = expired_.find(expired_time);
  if (iter != expired_.end()) {
    set_info = &iter->second;
  } else {
    expired_[expired_time] = SetString();
    set_info = &expired_[expired_time];
  }

  MapString::iterator map_iter;
  for (const auto& key : keys) {
    map_iter = keys_info_.find(key);
    if (map_iter != keys_info_.end()) {
      expired_[map_iter->second].erase(key);
    }
    keys_info_[key] = expired_time;
    set_info->insert(key);
  }
}

void Expired::SetExpired(const GoogleMap& key_values, int32_t ttl) {
  int32_t current_time = butil::gettimeofday_s();
  int32_t expired_time = current_time + ttl;
  SetString* set_info = NULL;

  butil::AutoLock auto_lock(lock_);
  auto iter = expired_.find(expired_time);
  if (iter != expired_.end()) {
    set_info = &iter->second;
  } else {
    expired_[expired_time] = SetString();
    set_info = &expired_[expired_time];
  }

  MapString::iterator map_iter;
  for (const auto& value : key_values) {
    map_iter = keys_info_.find(value.first);
    if (map_iter != keys_info_.end()) {
      expired_[map_iter->second].erase(value.first);
    }
    keys_info_[value.first] = expired_time;
    set_info->insert(value.first);
  }
}

void Expired::MigrateExpired(const GoogleMap& key_values) {
  ValueInfo value_info;
  butil::AutoLock auto_lock(lock_);
  for (const auto& value : key_values) {
    value_info.ParseFromString(value.second);
    if (value_info.expired()) {
      SetExpired(value.first, value_info.expired());
    }
  }
}

void Expired::CheckExpired(ArrayString* keys) {
  keys->clear();
  int32_t current_time = butil::gettimeofday_s();
  butil::AutoLock auto_lock(lock_);
  auto iter = expired_.begin();
  while (iter != expired_.end() && iter->first <= current_time) {
    for (const auto& key : iter->second) {
      keys->push_back(key);
      keys_info_.erase(key);
    }
    iter = expired_.erase(iter);
  }
}

void Expired::SetExpired(const std::string& key, int32_t expired) {
  SetString* set_info = NULL;
  auto iter = expired_.find(expired);
  if (iter != expired_.end()) {
    set_info = &iter->second;
  } else {
    expired_[expired] = SetString();
    set_info = &expired_[expired];
  }

  auto map_iter = keys_info_.find(key);
  if (map_iter != keys_info_.end()) {
    expired_[map_iter->second].erase(key);
  }
  keys_info_[key] = expired;
  set_info->insert(key);
}

}  // namespace node
}  // namespace noah
