// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_TEST_CLIENT_ITERATOR_H_
#define NOAH_TEST_CLIENT_ITERATOR_H_

#include <string>
#include <map>
#include <vector>
#include <unordered_map>

#include "common/entity.h"

typedef std::unordered_map<std::string, std::string> KeyValueMap;

namespace noah {
namespace test_client {
class Cluster;

struct SplitInfo {
  SplitInfo() : id(0), offset(0), size(0) {
  }

  int32_t id;
  int32_t offset;
  int32_t size;
  std::string last_key;
};

typedef std::vector<SplitInfo> VectorSplitInfo;
typedef noah::common::Status   Status;

class Iterator {
 public:
  typedef std::vector<int32_t> VectorInt;


  Iterator(const std::string& table_name, Cluster* cluster);
  ~Iterator();

  int32_t Size();
  Status List(int32_t offset, int32_t size, KeyValueMap* key_values, bool* end);

 private:
  Status GetTableInfo();
  void SplitSection(int32_t offset, int32_t size, VectorSplitInfo* split_infos,
                    bool* ended);

  std::string   table_name_;
  Cluster*      cluster_;
  int32_t       last_id_;
  int32_t       last_end_;
  std::string   last_key_;
  VectorInt     table_info_;
};
}  // namespace client
}  // namespace noah
#endif
