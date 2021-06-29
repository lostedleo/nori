// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_CLIENT_OPTIONS_H_
#define NOAH_CLIENT_OPTIONS_H_

#include <string>

namespace noah {
namespace client {

struct Options{
  Options();
  Options(const std::string& group, const std::string& conf);

  void DefaultInit();

  std::string     meta_group;             // MetaServer group name
  std::string     meta_conf;              // MetaServer configuration
  bool            create_table;           // If table not exist auto create table
  int32_t         auto_partition_num;     // Auto create table partition number
  int32_t         auto_duplicate_num;     // Auto create table duplicate numbber
  int64_t         auto_capacity;          // Auto create table capacity size
  int32_t         connect_timeout_ms;     // Connnect to server timeout in ms
  int32_t         request_timeout_ms;     // Request to server timeout in ms
  int32_t         retry_num;              // Rqueest retry number
};

}  // namespace client
}  // namespace noah
#endif
