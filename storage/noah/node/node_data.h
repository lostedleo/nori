// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_NODE_DATA_H_
#define NOAH_NODE_DATA_H_

#include <string>

#include <memory>
#include <unordered_map>
#include <vector>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/map.h>

#include "butil/synchronization/rw_mutex.h"

namespace noah {
namespace common {
class Table;
}
}

namespace noah {
namespace node {
class Cache;
class Expired;
class NodeServer;
class Ldb;
struct ListInfo;

typedef std::vector<noah::common::Table> TableArray;
typedef std::shared_ptr<Cache> CachePtr;
typedef std::unordered_map<std::string, CachePtr> CacheMap;
typedef std::vector<std::string> ArrayString;

typedef std::shared_ptr<Ldb> LdbPtr;
typedef std::unordered_map<std::string, LdbPtr> LdbMap;

std::string TableInfoString(const std::string& table_name, int32_t id);

class NodeData {
 public:
  typedef ::google::protobuf::Map<std::string, std::string> GoogleMap;
  typedef ::google::protobuf::RepeatedPtrField<std::string> GoogleRepeated;

  explicit NodeData(NodeServer* node_server);
  ~NodeData();

  bool Get(const std::string& table_name, int32_t id, const GoogleRepeated& keys,
           GoogleRepeated* values);
  bool Set(const std::string& table_name, int32_t id, const GoogleMap& key_values,
           int32_t ttl = -1);
  bool Set(const std::string& table_name, int32_t id, const GoogleMap& key_values,
           const std::vector<int32_t>& ttl_vec);
  bool Del(const std::string& table_name, int32_t id, const GoogleRepeated& keys);
  bool Migrate(const std::string& table_name, int32_t id,
               const GoogleMap& key_values, bool expired);
  bool List(const std::string& table_name, int32_t id, const ListInfo& info,
            GoogleMap* key_values, int32_t* total_size, bool* end,
            std::string* last_key);
  CachePtr GetCache(const std::string& table_name, int32_t id, bool create = false);
  void CheckExpired();
  void ClearTables(const TableArray& tables, CacheMap* caches);
  int32_t GetCacheSize(const std::string& table_name, int32_t id);

  LdbPtr GetLdb(const std::string& table_name, int32_t id, bool create = false);
  bool LdbSet(const std::string table_name, int32_t id, const GoogleMap& key_values,
              int32_t expired);
  bool LdbDel(const std::string table_name, int32_t id, const GoogleRepeated& keys);
  bool LdbGet(const std::string& table_name, int32_t id, const GoogleRepeated& keys,
              GoogleRepeated* values);
  bool LoadDataFormDb();
  void CheckLdbExpired();
  void ClearLdbTables(const TableArray& tables);

 private:
  NodeServer*     node_server_;
  butil::RWMutex  lock_;
  CacheMap        caches_;
  LdbMap          ldbs_;
};

}  // namespace node
}  // namespace noah
#endif
