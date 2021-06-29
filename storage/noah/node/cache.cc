// Author: Zhenwei Zhu losted.leo@gmail.com

#include "node/cache.h"

#include <gflags/gflags.h>

#include "butil/time.h"

#include "node/node.pb.h"

#include "butil/logging.h"
DEFINE_int32(batch_expired_size, 100 * 1000, "Batch check expired key size");
DEFINE_int32(expired_sleep_ms, 1, "Batch check expired key sleep in ms");

namespace noah {
namespace node {
Cache::Cache(int64_t capacity /*= 0*/) : expired_(false) {
  if (capacity) {
    key_values_.reserve(capacity);
  }
}

bool Cache::Get(const GoogleRepeated& keys, GoogleRepeated* values) {
  ValueInfo value_info;
  int32_t current_time = butil::gettimeofday_s();
  butil::ReaderAutoLock lock_r(&lock_);
  Map::const_iterator iter;
  for (const auto& value : keys) {
    iter = key_values_.find(value);
    if (iter != key_values_.end()) {
      value_info.ParseFromString(iter->second);
      if (value_info.expired() && value_info.expired() <= current_time) {
        *(values->Add()) = std::string();
      } else {
        *(values->Add()) = value_info.value();
      }
    } else {
      *(values->Add()) = std::string();
    }
  }

  return true;
}

bool Cache::Set(const GoogleMap& key_values, int32_t ttl) {
  ValueInfo value_info;
  if (ttl > 0) {
    int32_t current_time = butil::gettimeofday_s();
    int32_t expired = current_time + ttl;
    value_info.set_expired(expired);
    expired_ = true;
  }
  butil::WriterAutoLock lock_w(&lock_);
  for (const auto& value : key_values) {
    value_info.set_value(value.second);
    key_values_[value.first] = value_info.SerializeAsString();
  }

  return true;
}

bool Cache::Set(const GoogleMap& key_values, const std::vector<int32_t>& ttl_vec) {
  if (key_values.size() != ttl_vec.size()) {
    LOG(INFO) << "key_value size != ttl size";
    return false;
  }

  int32_t ttl_size = ttl_vec.size();
  butil::WriterAutoLock lock_w(&lock_);
  int32_t i = 0;
  for (const auto& value : key_values) {
    ValueInfo value_info;
    if (i >= ttl_size) {
      break;
    }
    int32_t ttl = ttl_vec[i++];
    if (ttl > 0) {
      int32_t current_time = butil::gettimeofday_s();
      int32_t expired = current_time + ttl;
      value_info.set_expired(expired);
      expired_ = true;
    }
    value_info.set_value(value.second);
    key_values_[value.first] = value_info.SerializeAsString();
  }
  return true;
}

bool Cache::Del(const GoogleRepeated& keys) {
  butil::WriterAutoLock lock_w(&lock_);
  for (const auto& key : keys) {
    key_values_.erase(key);
  }
  return true;
}

bool Cache::Del(const ArrayString& keys) {
  butil::WriterAutoLock lock_w(&lock_);
  for (const auto& key : keys) {
    key_values_.erase(key);
  }
  return true;
}

bool Cache::Migrate(const GoogleMap& key_values, bool expired) {
  butil::WriterAutoLock lock_w(&lock_);
  for (const auto& value : key_values) {
    key_values_[value.first] = value.second;
  }
  if (expired) {
    expired_ = true;
  }
  return true;
}

bool Cache::List(const ListInfo& info, GoogleMap* key_values, int32_t* total_size,
                 bool* end, std::string* last_key) {
  ValueInfo value_info;
  int32_t current_time = butil::gettimeofday_s();

  // First find offset
  butil::ReaderAutoLock lock_r(&lock_);
  auto iter = key_values_.end();
  if (!info.offset_key.empty()) {
    iter = key_values_.find(info.offset_key);
  }
  if (iter == key_values_.end()) {
    int32_t index = 0;
    iter = key_values_.begin();
    for (; iter != key_values_.end(); ++iter) {
      if (index++ >= info.offset) {
        break;
      }
    }
  }

  // Second output size of keys
  int32_t index_size = 0;
  for (; iter != key_values_.end(); ++iter) {
    if (index_size++ >= info.size) {
      break;
    }
    value_info.ParseFromString(iter->second);
    if (!value_info.expired() || value_info.expired() > current_time) {
      (*key_values)[iter->first] = value_info.value();
    }
  }
  if (iter == key_values_.end()) {
    *end = true;
  } else {
    *end = false;
    *last_key = iter->first;
  }
  *total_size = key_values_.size();

  return true;
}

bool Cache::Clear() {
  butil::WriterAutoLock lock_w(&lock_);
  expired_ = false;
  Map em_map;
  key_values_.swap(em_map);

  key_values_.clear();
  return true;
}

int64_t Cache::CheckExpired(int64_t* size) {
  int64_t expired_size = 0;
  if (!expired_) {
    if (size) {
      *size = 0;
    }
    return expired_size;
  }

  {
    butil::WriterAutoLock lock_w(&lock_);
    if (size) {
      *size = key_values_.size();
    }
  }

  ValueInfo value_info;
  std::string last_key;
  Map::iterator iter;
  while (true) {
    int32_t current_time = butil::gettimeofday_s();
    int32_t checked_size = 0;
    {
      butil::WriterAutoLock lock_w(&lock_);
      if (!last_key.empty()) {
        iter = key_values_.find(last_key);
        if (iter == key_values_.end()) {
          iter = key_values_.begin();
        }
      } else {
        iter = key_values_.begin();
      }
      while (iter != key_values_.end()) {
        value_info.ParseFromString(iter->second);
        if (value_info.expired() && value_info.expired() <= current_time) {
          iter = key_values_.erase(iter);
          expired_size++;
        } else {
          ++iter;
        }
        ++checked_size;
        if (checked_size >= FLAGS_batch_expired_size) {
          break;
        }
      }
      if (iter != key_values_.end()) {
        last_key = iter->first;
      } else {
        break;
      }
    }
    usleep(FLAGS_expired_sleep_ms * 1000);
  }
  return expired_size;
}

}  // namespace node
}  // namespace noah
