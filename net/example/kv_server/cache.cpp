#include "cache.h"

namespace push {
bool Cache::Get(const GoogleRepeated& keys, GoogleRepeated* values) const {
  butil::AutoLock auto_lock(lock_);
  Map::const_iterator iter;
  for (int i = 0; i < keys.size(); ++i) {
  iter = key_values_.find(keys.Get(i));
  if (iter != key_values_.end()) {
    *(values->Add()) = iter->second;
  } else {
    *(values->Add()) = std::string();
  }
  }

  return true;
}

bool Cache::Set(const GoogleMap& key_values) {
  butil::AutoLock auto_lock(lock_);
  GoogleMap::const_iterator iter;
  for (iter = key_values.begin(); iter != key_values.end(); iter++) {
  key_values_[iter->first] = iter->second;
  }

  return true;
}
}  // namespace push
