// Author: Zhenwei Zhu losted.leo@gmail.com

#include "node/ldb.h"
#include "butil/time.h"
#include "node/node.pb.h"

#include "butil/logging.h"
#include "common/util.h"

DEFINE_string(leveldb_path, "/data/news/data/noah_data/", "level db path");
DEFINE_string(trash_path, "/data/news/data/noah_trash_data/", "level db trash path");
DEFINE_string(part_pre, "NoahPart_", "level db part pre");
namespace noah {
namespace node {

  Ldb::Ldb(std::string table_name, int id) {
    table_name_ = table_name;
    id_ = id;
    db_options_.create_if_missing = true;
    db_options_.filter_policy = leveldb::NewBloomFilterPolicy(10);
    db_options_.write_buffer_size = 1024*1024*1024;
    char buf[256];
    std::string path = "%s/" + fLS::FLAGS_part_pre + "%u/";
    snprintf(buf, sizeof(buf), path.c_str(), table_name_.c_str(), id_);
    db_path_ = FLAGS_leveldb_path + std::string(buf);
    trash_path_ = FLAGS_trash_path;
    open_status_ = leveldb::DB::Open(db_options_, db_path_.c_str(), &db_);
}

bool Ldb::Get(const GoogleRepeated& keys, GoogleRepeated* values) {
  if (NULL == db_) {
    LOG(INFO) << "db is null ";
    return false;
  }

  if (!open_status_.ok()) {
    LOG(INFO) << db_path_ << " create leveldb failed!";
    return false;
  }
  ValueInfo value_info;
  for (const auto& key : keys) {
    std::string value;
    std::string encode_key;
    if (!noah::common::EncodeLbKey(key, table_name_, id_,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      LOG(INFO) << "LDB get encode failed " << table_name_ << " key " << key;
      *(values->Add()) = std::string();
      continue;
    }

    leveldb::Status  s = db_->Get(read_options_, encode_key, &value);
    if (s.ok()) {
      value_info.ParseFromString(value);
      *(values->Add()) = value_info.value();
    } else {
      *(values->Add()) = std::string();
    }
  }

  return true;
}

bool Ldb::Set(const GoogleMap& key_values, int32_t ttl) {
  if (NULL == db_) {
    LOG(INFO) << "db is null ";
    return false;
  }

  if (!open_status_.ok()) {
    LOG(INFO) << db_path_ << " create leveldb failed!";
    return false;
  }

  ValueInfo value_info;
  if (ttl > 0) {
    int32_t current_time = butil::gettimeofday_s();
    int32_t expired = current_time + ttl;
    value_info.set_expired(expired);
  }
  butil::WriterAutoLock lock_w(&lock_);
  leveldb::WriteBatch batch;
  for (const auto& value : key_values) {
    value_info.set_value(value.second);
    std::string encode_key;
    if (!noah::common::EncodeLbKey(value.first, table_name_, id_,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      LOG(INFO) << "Encode failed! " << table_name_ << " key" << value.first;
      continue;
    }
    batch.Put(encode_key, value_info.SerializeAsString());
  }

  leveldb::Status s = db_->Write(write_options_, &batch);
  if (!s.ok()) {
    LOG(INFO) << "Ldb set failed  " << table_name_;
    return false;
  }

  return true;
}

bool Ldb::Set(const GoogleMap& key_values, const std::vector<int32_t>& ttl_vec) {
  if (NULL == db_) {
    LOG(INFO) << "db is null ";
    return false;
  }

  if (!open_status_.ok()) {
    LOG(INFO) << db_path_ << " create leveldb failed!";
    return false;
  }

  if (key_values.size() != ttl_vec.size()) {
    LOG(INFO) << "key_value size != ttl size";
    return false;
  }

  int32_t ttl_size = ttl_vec.size();
  butil::WriterAutoLock lock_w(&lock_);
  int32_t i = 0;
  leveldb::WriteBatch batch;
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
    }
    value_info.set_value(value.second);
    std::string encode_key;
    if (!noah::common::EncodeLbKey(value.first, table_name_, id_,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      LOG(INFO) << "Encode failed! " << table_name_ << " key" << value.first;
      continue;
    }
    batch.Put(encode_key, value_info.SerializeAsString());
  }

  leveldb::Status s = db_->Write(write_options_, &batch);
  if (!s.ok()) {
    LOG(INFO) << "Ldb set failed  " << table_name_;
    return false;
  }
  return true;
}

bool Ldb::Del(const GoogleRepeated& keys) {
  if (NULL == db_) {
    LOG(INFO) << "db is null ";
    return false;
  }

  if (!open_status_.ok()) {
    LOG(INFO) << db_path_ << " create leveldb failed!";
    return false;
  }

  butil::WriterAutoLock lock_w(&lock_);
  leveldb::WriteBatch batch;
  for (const auto& key : keys) {
    std::string value;
    std::string encode_key;
    if (!noah::common::EncodeLbKey(key, table_name_, id_,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      continue;
    }
    batch.Delete(encode_key);
  }

  leveldb::Status s = db_->Write(write_options_, &batch);
  if (!s.ok()) {
    return false;
  }

  return true;
}

bool Ldb::Clear() {
  butil::WriterAutoLock lock_w(&lock_);
  LOG(INFO) << "enter Clear " << db_path_;
  if (NULL != db_) {
    delete db_;
  }

  LOG(INFO) << "Clear " << db_path_;
  return true;
}

int64_t Ldb::CheckExpired(int64_t* size) {
  *size = 0;
  if (NULL == db_) {
    LOG(INFO) << "db is null ";
    return 0;
  }

  if (!open_status_.ok()) {
    LOG(INFO) << db_path_ << " create leveldb failed!";
    return 0;
  }

  int64_t expired_size = 0;
  std::vector<std::string> keys;
  uint32_t batch_size = 10000;

  butil::WriterAutoLock lock_w(&lock_);
  leveldb::ReadOptions read_options;
  leveldb::WriteOptions write_options;
  const leveldb::Snapshot* snapshot = db_->GetSnapshot();
  read_options.snapshot =  snapshot;
  leveldb::Iterator* it = db_->NewIterator(read_options);
  keys.clear();

  (*size) = 0;
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    (*size)++;
    ValueInfo value_info;
    std::string key_str = it->key().ToString();
    std::string value_str = it->value().ToString();
    int32_t current_time = butil::gettimeofday_s();
    value_info.ParseFromString(value_str);
    if (value_info.expired() && value_info.expired() <= current_time) {
      keys.push_back(key_str);

      expired_size++;
      if (keys.size() >= batch_size) {
        leveldb::WriteBatch batch;
        for (const auto& key : keys) {
          batch.Delete(key);
        }
        leveldb::Status s = db_->Write(write_options, &batch);
        keys.clear();
      }
    }
  }
  delete it;
  leveldb::WriteBatch batch;
  for (const auto& key : keys) {
    batch.Delete(key);
  }

  leveldb::Status s = db_->Write(write_options, &batch);
  if (!s.ok()) {
    LOG(INFO) << "delete db " << table_name_ << " " << id_ <<  " falied!";
  }
  db_->ReleaseSnapshot(read_options.snapshot);
  return expired_size;
}

}  // namespace node
}  // namespace noah
