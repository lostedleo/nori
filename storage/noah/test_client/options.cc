// Author: Zhenwei Zhu losted.leo@gmail.com

#include "test_client/options.h"

namespace noah {
namespace test_client {
Options::Options() {
  DefaultInit();
}

Options::Options(const std::string& group, const std::string& conf)
  : meta_group(group), meta_conf(conf) {
  DefaultInit();
}

void Options::DefaultInit() {
  create_table       = true;
  auto_partition_num = 1;
  auto_duplicate_num = 2;
  auto_capacity      = 0;
  connect_timeout_ms = 1000;
  request_timeout_ms = 1000;
  retry_num          = 2;
}
}  // namespace client
}  // namespace noah
