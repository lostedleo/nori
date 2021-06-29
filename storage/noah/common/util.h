#ifndef NOAH_COMMON_UTIL_H
#define NOAH_COMMON_UTIL_H

#include <iostream>
#include <string>
#include <vector>

namespace noah {
namespace common {
  bool GetIpAddress(std::string* strIp, const std::string& strEth);
  bool EncodeLbKey(const std::string& key, const std::string& table_name, int id, char key_type,
                   std::string* encode_key);
  bool DecodeLbKey(const std::string& key, std::string* table_name, int* id, char* key_type,
                   std::string* decode_key);
  bool GetDirList(const std::string& strDir, std::vector<std::string>* vResultList);
}  // namespace common
}  // namespace noah

#endif
