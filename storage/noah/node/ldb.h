// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_LDB_H_
#define NOAH_NODE_LDB_H_

#include <string>
#include <unordered_map>
#include <vector>

#include <gflags/gflags.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/map.h>

#include "butil/synchronization/rw_mutex.h"

#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/write_batch.h"
#include "common/util.h"
#include "common/entity.h"



namespace noah {
namespace node {

class Ldb {
 public:
  typedef ::google::protobuf::Map<std::string, std::string> GoogleMap;
  typedef ::google::protobuf::RepeatedPtrField<std::string> GoogleRepeated;

  Ldb(std::string table_name, int id);
  ~Ldb() {}
  bool Get(const GoogleRepeated& keys, GoogleRepeated* values);
  bool Set(const GoogleMap& key_values, int32_t ttl = -1);
  bool Set(const GoogleMap& key_values, const std::vector<int32_t>& ttl_vec);
  bool Del(const GoogleRepeated& keys);
  bool Clear();

  int64_t CheckExpired(int64_t* size);
  inline leveldb::Status GetOpenStatus() { return open_status_; }
  inline leveldb::DB* GetDb() { return db_; }

 private:
  butil::RWMutex lock_;
  leveldb::DB* db_;
  leveldb::Options db_options_;
  leveldb::Status open_status_;
  leveldb::WriteOptions write_options_;
  leveldb::ReadOptions read_options_;
  std::string db_path_;
  std::string trash_path_;
  std::string table_name_;
  int id_;
};
}  // namespace node
}  // namespace noah

#endif
