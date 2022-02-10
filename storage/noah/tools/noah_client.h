// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_TOOL_CLIENT_H_
#define NOAH_TOOL_CLIENT_H_

#include <string>
#include <unordered_map>

namespace noah {
namespace common {
class Table;
struct MetaInfo;
}

namespace meta {
class MetaNodes;
}

}

typedef std::unordered_map<std::string, std::string> KeyValueMap;

namespace noah {
namespace client {
class Cluster;
class Iterator;

class NoahClient {
 public:
  explicit NoahClient(Cluster* cluster);
  ~NoahClient();

  Cluster* cluster() { return cluster_; }
  bool FindTable(const std::string& table_name, noah::common::Table* info);
  bool Set(const std::string& table_name, const std::string& key,
           const std::string& value, int32_t ttl = -1);
  bool Get(const std::string& table_name, const std::string& key, std::string* value);
  bool Del(const std::string& table_name, const std::string& key);
  bool Info(noah::common::MetaInfo** info);
  bool GetMetaNodes(noah::meta::MetaNodes* meta_nodes);
  bool DropTable(const std::string& table_name);
  Iterator* NewIterator(const std::string& table_name);

 private:
  Cluster*    cluster_;
};
}  // namespace client
}  // namespace noah
#endif
