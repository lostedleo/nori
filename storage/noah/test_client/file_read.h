// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_TEST_CLIENT_FILE_READ_H_
#define NOAH_TEST_CLIENT_FILE_READ_H_

#include <vector>
#include <string>

#include "butil/files/scoped_file.h"

namespace noah {
namespace test_client {
class FileRead {
 public:
  typedef std::vector<std::string> Array;

  explicit FileRead(const char* path);
  ~FileRead();

  int Read(int64_t offset, int32_t size, Array* keys, Array* values);

 private:
  butil::ScopedFILE   file_;
};
}  // namespace client
}  // namespace noah
#endif
