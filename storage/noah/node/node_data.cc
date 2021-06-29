// Author: Zhenwei Zhu losted.leo@gmail.com

#include "node/node_data.h"

#include "butil/strings/string_number_conversions.h"
#include "butil/logging.h"
#include "butil/time.h"

#include "common/entity.h"

#include "node/node_server.h"
#include "node/node.pb.h"
#include "node/cache.h"
#include "node/ldb.h"
#include "node/expired.h"
#include <sstream>

DECLARE_string(leveldb_path);
DECLARE_string(trash_path);
DECLARE_string(part_pre);
namespace noah {
namespace node {
std::string TableInfoString(const std::string& table_name, int32_t id) {
  return table_name + "_" + butil::IntToString(id);
}

NodeData::NodeData(NodeServer* node_server)
  : node_server_(node_server) {
}

NodeData::~NodeData() {
}

bool NodeData::Get(const std::string& table_name, int32_t id,
                   const GoogleRepeated& keys, GoogleRepeated* values) {
  CachePtr cache = GetCache(table_name, id);
  values->Reserve(keys.size());
  {
    if (cache.get()) {
      return cache->Get(keys, values);
    } else {
      for (int i = 0; i < keys.size(); ++i) {
        *values->Add() = std::string();
      }
    }
  }
  return true;
}

bool NodeData::Set(const std::string& table_name, int32_t id,
                   const GoogleMap& key_values, int32_t ttl) {
  CachePtr cache = GetCache(table_name, id, true);
  return cache->Set(key_values, ttl);
}

bool NodeData::Set(const std::string& table_name, int32_t id, const GoogleMap& key_values,
    const std::vector<int32_t>& ttl_vec) {
  CachePtr cache = GetCache(table_name, id, true);
  return cache->Set(key_values, ttl_vec);
}

bool NodeData::Del(const std::string& table_name, int32_t id,
                   const GoogleRepeated& keys) {
  CachePtr cache = GetCache(table_name, id);
  if (cache.get()) {
    return cache->Del(keys);
  }
  return true;
}

bool NodeData::Migrate(const std::string& table_name, int32_t id,
                       const GoogleMap& key_values, bool expired) {
  CachePtr cache = GetCache(table_name, id, true);
  return cache->Migrate(key_values, expired);
}

bool NodeData::List(const std::string& table_name, int32_t id, const ListInfo& info,
                    GoogleMap* key_values, int32_t* total_size, bool* end,
                    std::string* last_key) {
  CachePtr cache = GetCache(table_name, id);
  if (cache.get()) {
    return cache->List(info, key_values, total_size, end, last_key);
  } else {
    *total_size = 0;
    *end = true;
  }
  return true;
}

int32_t NodeData::GetCacheSize(const std::string& table_name, int32_t id) {
  CachePtr cache = GetCache(table_name, id);
  if (cache.get()) {
    return cache->Size();
  }
  return 0;
}

CachePtr NodeData::GetCache(const std::string& table_name, int32_t id,
                            bool create /*=false*/) {
  std::string cache_str = TableInfoString(table_name, id);
  {
    butil::ReaderAutoLock lock_r(&lock_);
    auto iter = caches_.find(cache_str);
    if (iter != caches_.end()) {
      return iter->second;
    }
  }
  if (create) {
    butil::WriterAutoLock lock_w(&lock_);
    auto iter = caches_.find(cache_str);
    if (iter != caches_.end()) {
      return iter->second;
    }
    noah::common::Table table;
    int64_t capacity = 0;
    if (node_server_->FindTable(table_name, &table)) {
      capacity = table.capacity() / table.partition_num();
    }
    caches_[cache_str] = CachePtr(new Cache(capacity));
    LOG(INFO) << "create talbe name = " << cache_str;
    return caches_[cache_str];
  }
  return CachePtr();
}

void NodeData::CheckExpired() {
  ArrayString keys;
  butil::Timer timer;
  timer.start();
  int64_t expired_size = 0;
  int64_t total_size = 0;
  int64_t partition_size = 0;
  CacheMap snapshot;
  {
    butil::ReaderAutoLock lock_r(&lock_);
    snapshot = caches_;
  }
  for (const auto& value : snapshot) {
    expired_size += value.second->CheckExpired(&partition_size);
    total_size += partition_size;
  }
  timer.stop();
  LOG(INFO) << "CheckExpired tatal size " << total_size << " expired size "
            << expired_size << " cost " << timer.n_elapsed() << " ns";
}

void NodeData::ClearTables(const TableArray& tables, CacheMap* caches) {
  std::string cache_str;
  butil::WriterAutoLock lock_w(&lock_);
  for (const auto& table : tables) {
    int partition_num = table.partition_num();
    for (int i = 0; i < partition_num; ++i) {
      cache_str = TableInfoString(table.table_name(), i);
      auto iter = caches_.find(cache_str);
      if (iter != caches_.end()) {
        (*caches)[cache_str] = iter->second;
        caches_.erase(iter);
      }
    }
  }
}

LdbPtr NodeData::GetLdb(const std::string& table_name, int32_t id, bool create) {
  std::string cache_str = TableInfoString(table_name, id);
  {
    butil::ReaderAutoLock lock_r(&lock_);
    auto iter = ldbs_.find(cache_str);
    if (iter != ldbs_.end()) {
      return iter->second;
    }
  }

  if (create) {
    butil::WriterAutoLock lock_w(&lock_);
    auto iter = ldbs_.find(cache_str);
    if (iter != ldbs_.end()) {
      return iter->second;
    }
    LdbPtr ldb_ptr = LdbPtr(new Ldb(table_name, id));
    if (!ldb_ptr->GetOpenStatus().ok()) {
      LOG(INFO) << "create leveldb table name = " << cache_str << " failed!";
      return NULL;
    }
    ldbs_[cache_str] = ldb_ptr;
    return ldbs_[cache_str];
  }

  return NULL;
}

bool NodeData::LdbSet(const std::string table_name, int32_t id, const GoogleMap& key_values,
                      int32_t expired) {
  LdbPtr ldb = GetLdb(table_name, id, true);
  if (NULL == ldb) {
    LOG(INFO) << "create table " << table_name <<  " id " << id << " failed!";
    return false;
  }
  return ldb->Set(key_values, expired);
}

bool NodeData::LdbGet(const std::string& table_name, int32_t id, const GoogleRepeated& keys,
                      GoogleRepeated* values) {
  LdbPtr ldb = GetLdb(table_name, id);
  if (NULL == ldb) {
    LOG(INFO) << "Get table " << table_name <<  " id " << id << " leveldb get failed!";
    return false;
  }

  values->Reserve(keys.size());
  {
    if (ldb.get()) {
      return ldb->Get(keys, values);
    } else {
      for (int i = 0; i < keys.size(); ++i) {
        *values->Add() = std::string();
      }
    }
  }

  return true;
}

bool NodeData::LdbDel(const std::string table_name, int32_t id, const GoogleRepeated& keys) {
  LdbPtr ldb = GetLdb(table_name, id);
  if (NULL == ldb) {
    LOG(INFO) << "Get table " << table_name <<  " id " << id << " leveldb get failed! in LdbDel";
    return false;
  }

  return ldb->Del(keys);
}

void NodeData::CheckLdbExpired() {
  ArrayString keys;
  butil::Timer timer;
  timer.start();
  int64_t expired_size = 0;
  int64_t total_size = 0;
  int64_t partition_size = 0;
  LdbMap snapshot;
  {
    butil::ReaderAutoLock lock_r(&lock_);
    snapshot = ldbs_;
  }
  for (const auto& value : snapshot) {
    expired_size += value.second->CheckExpired(&partition_size);
    total_size += partition_size;
  }
  timer.stop();
  LOG(INFO) << "CheckLdbExpired tatal size " << total_size << " expired size "
    << expired_size << " cost " << timer.n_elapsed() << " ns";
}

bool NodeData::LoadDataFormDb() {
  GoogleMap key_values;
  std::vector<int32_t> ttl_vec;
  char type = 1;
  uint32_t batch_size = 1000000;

  std::vector<std::string> table_names;
  noah::common::GetDirList(FLAGS_leveldb_path, &table_names);
  if (table_names.size() == 0) {
    LOG(INFO) << "LOAD LevelDb from " << FLAGS_leveldb_path << " failed!";
    return false;
  }

  LOG(INFO) << "inter LOAD LevelDb from " << FLAGS_leveldb_path << " failed!";
  for (int i = 0; i < static_cast<int>(table_names.size()); i++) {
    std::string part_path = FLAGS_leveldb_path + table_names[i];
    std::vector<std::string> tmp_parts;
    noah::common::GetDirList(part_path, &tmp_parts);
    LOG(INFO) << "nt LOAD LevelDb from " << part_path << " tmp size " << tmp_parts.size();
    for (int j = 0; j < static_cast<int>(tmp_parts.size()); j++) {
      LOG(INFO) << "tmp par " << tmp_parts[j];
      std::size_t found = tmp_parts[j].find(FLAGS_part_pre);
      if (found != std::string::npos) {
        LOG(INFO) << " find ";
        std::stringstream ss;
        ss.str(std::string());
        ss << tmp_parts[j];
        std::string item;

        std::vector<std::string> elems;
        elems.clear();
        while (getline(ss, item, '_')) {
          elems.push_back(item);
        }
        if (elems.size() != 2) {
          LOG(INFO) << " elems size != 2";
          continue;
        }

        int part_id = stoi(elems[1]);
        LOG(INFO) << "part_path " << part_path << " tableName " << table_names[i] << " part "
          << part_id;
        LdbPtr ldb = GetLdb(table_names[i], part_id, true);
        if (!ldb.get()) {
          LOG(INFO) << "Get Ldb failed " << table_names[i] << " part " << part_id;
          continue;
        }

        leveldb::DB* db = ldb->GetDb();
        if (NULL == db) {
          LOG(INFO) << "db is null " << table_names[i] << " part " << part_id;
          continue;
        }

        leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
        key_values.clear();
        ttl_vec.clear();

        for (it->SeekToFirst(); it->Valid(); it->Next()) {
          std::string key_str = it->key().ToString();
          std::string value_str = it->value().ToString();
          std::string table_name, ori_key;

          int id;
          if (!noah::common::DecodeLbKey(key_str , &table_name, &id, &type, &ori_key)) {
            LOG(INFO) << "decode failled! " << key_str;
            continue;
          }

          if (table_name != table_names[i] || id != part_id) {
            LOG(INFO) << "decod key  " << ori_key << " get_key " << key_str <<  " table "
              << table_name << " id " << part_id <<  " value "<< value_str << " decode failed!";
            continue;
          }
          ValueInfo value_info;
          int32_t current_time = butil::gettimeofday_s();
          value_info.ParseFromString(value_str);
          if (value_info.expired() && value_info.expired() <= current_time) {
            continue;
          }

          key_values[ori_key] = value_info.value();
          ttl_vec.push_back(value_info.expired()? value_info.expired():-1);
          if (key_values.size() > batch_size) {
            Set(table_names[i], part_id, key_values, ttl_vec);
            key_values.clear();
            ttl_vec.clear();
          }
        }
        if (key_values.size() > 0) {
          Set(table_names[i], part_id, key_values, ttl_vec);
          key_values.clear();
          ttl_vec.clear();
        }
      }
    }
  }

  return true;
}

void NodeData::ClearLdbTables(const TableArray& tables) {
  std::string cache_str;
  butil::WriterAutoLock lock_w(&lock_);
  for (const auto& table : tables) {
    int partition_num = table.partition_num();
    for (int i = 0; i < partition_num; ++i) {
      cache_str = TableInfoString(table.table_name(), i);
      LOG(INFO) << "cache_str " << cache_str;
      auto iter = ldbs_.find(cache_str);
      if (iter != ldbs_.end()) {
        LOG(INFO) << " get cache_str " << cache_str;
        iter->second->Clear();
        ldbs_.erase(iter);
      }
    }
    std::string table_name_path = FLAGS_leveldb_path + table.table_name();
  }
}
}  // namespace node
}  // namespace noah
