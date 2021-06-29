// Author: Zhenwei Zhu losted.leo@gmail.com

#include "client/iterator.h"

#include "client/cluster.h"

namespace noah {
namespace client {
Iterator::Iterator(const std::string& table_name, Cluster* cluster)
  : table_name_(table_name), cluster_(cluster), last_id_(0), last_end_(0) {
}

Iterator::~Iterator() {
}


int32_t Iterator::Size() {
  Status status = GetTableInfo();
  if (Status::kOk != status) {
    return 0;
  }
  return table_info_.back();
}

Status Iterator::List(int32_t offset, int32_t size, KeyValueMap* key_values, bool* end) {
  key_values->clear();
  Status status = GetTableInfo();
  if (Status::kOk != status) {
    return status;
  }
  VectorSplitInfo split_infos;
  SplitSection(offset, size, &split_infos, end);
  if (split_infos.empty()) {
    *end = true;
    return status;
  }
  status = cluster_->ListTable(table_name_, split_infos, key_values, &last_key_);

  return status;
}

Status Iterator::GetTableInfo() {
  if (table_info_.size()) {
    return Status::kOk;
  }
  Status status = cluster_->ListTableInfo(table_name_, &table_info_);
  if (Status::kOk == status) {
    return status;
  }
  table_info_.clear();
  return status;
}

void Iterator::SplitSection(int32_t offset, int32_t size, VectorSplitInfo* split_infos,
                            bool* ended) {
  *ended = false;
  int32_t start = offset;
  int32_t end = offset + size;
  SplitInfo info;
  size_t i = 0;
  if (offset >= last_end_) {
    i = last_id_;
  }
  if (offset == last_end_) {
    info.last_key = last_key_;
  } else {
    last_key_.clear();
  }
  for (; i < table_info_.size(); ++i) {
    if (start < table_info_[i]) {
      info.id = i;
      if (i >= 1) {
        info.offset = start - table_info_[i - 1];
      } else {
        info.offset = start;
      }
      if (end <= table_info_[i]) {
        info.size = end - start;
        last_id_ = i;
        split_infos->push_back(info);
        break;
      }
      info.size = table_info_[i] - start;
      start = table_info_[i];
      split_infos->push_back(info);
      info.last_key.clear();
    }
  }
  last_end_ = end;

  if (table_info_.size()) {
    if (end >= table_info_.back()) {
      *ended = true;
    }
  }
}

}  // namespace client
}  // namespace noah
