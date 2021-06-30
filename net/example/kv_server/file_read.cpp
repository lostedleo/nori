#include "file_read.h"

#include <butil/strings/string_split.h>

namespace push {
FileRead::FileRead(const char* path) : file_(path, "r") {
}

FileRead::~FileRead() {
}

int FileRead::Read(int64_t offset, int32_t size, Array* keys, Array* values) {
  int read_size = 0;
  FILE* fd = file_.get();
  if (!fd) {
  LOG(ERROR) << "Fail to open file";
  return read_size;
  }
  char buf[4096];
  int64_t line = 0;
  std::vector<std::string> infos;
  while (fgets(buf, sizeof(buf), fd)) {
  if (line < offset) {
    line++;
    continue;
  } else if (line >= offset + size) {
    break;
  }
  line++;

  buf[strlen(buf) - 1] = '\0';
  butil::SplitString(std::string(buf, strlen(buf)), ',', &infos);
  if (infos.size() == 3) {
    keys->push_back(infos[0]);
    values->push_back(infos[1]);
    read_size++;
  }
  }

  return read_size;
}
}  // namespace push
