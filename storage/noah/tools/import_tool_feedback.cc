// Author: Zhenwei Zhu losted.leo@gmail.com

#include <fstream>
#include <sstream>
#include <unistd.h>

#include <gflags/gflags.h>

#include "bthread/bthread.h"
#include "butil/logging.h"
#include "butil/time.h"
#include "bvar/bvar.h"
#include "butil/files/file_path.h"
#include "butil/files/memory_mapped_file.h"
#include "butil/strings/string_split.h"

#include "client/cluster.h"

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_string(meta_group, "MetaServer", "MetaServer group name");
DEFINE_string(meta_conf, "", "MetaServer configuration");
DEFINE_string(table_name, "", "Table name");
DEFINE_int32(partition_num, 1, "Table Partition Number");
DEFINE_int32(duplicate_num, 2, "Table Duplicate Number");
DEFINE_int64(capacity, 10 * 1000 * 1000, "Table capacity size");
DEFINE_string(data_file, "", "File to read data");
DEFINE_int64(offset, 0, "Offset of lines to read file");
DEFINE_int32(size, 1000*1000, "Size of key_values");
DEFINE_int32(batch_size, 100, "Batch request size");
DEFINE_int32(ttl, -1, "Set value expired time in second");
DEFINE_int32(type, 0, "Operation type is set or get or list");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  noah::client::Options options(FLAGS_meta_group, FLAGS_meta_conf);
  noah::client::Cluster cluster(options);
  if (!cluster.Init()) {
    LOG(ERROR) << "Init cluster error";
    return -1;
  }

  std::string table_name = FLAGS_table_name;
  noah::common::Status status = cluster.CreateTable(table_name, FLAGS_partition_num,
                                                    FLAGS_duplicate_num, FLAGS_capacity);
  if (noah::common::Status::kOk != status &&
      noah::common::Status::kExist != status) {
    LOG(ERROR) << "Create table " << table_name << " error " << status;
    return -2;
  }

  if (FLAGS_data_file.empty()) {
    LOG(ERROR) << "Please input file_name";
    return -1;
  }

  KeyValueMap key_values;
  std::string line;
  std::ifstream fin(FLAGS_data_file);
  while (getline(fin, line)) {
    if (key_values.size() >= (uint32_t)FLAGS_batch_size) {
      int64_t begin_time_us = butil::gettimeofday_us();
      noah::common::Status status = cluster.Set(FLAGS_table_name, key_values, FLAGS_ttl);
      int64_t expired_us = butil::gettimeofday_us() - begin_time_us;
      if (noah::common::Status::kOk == status) {
        LOG(INFO) << "succ to" << FLAGS_table_name << " size:" << key_values.size() << " cost:" << expired_us << "us";
      } else {
        LOG(ERROR) << "failed to " << FLAGS_table_name << " status:" << status;
      }
      key_values.clear();
    }


    std::vector<std::string> infos;
    butil::SplitString(line, '&', &infos);
    if (infos.size() < 2) {
      LOG(WARNING) << "the format of line is error, line:" << line;
      continue;
    }
    if (infos[0].empty() || infos[1].empty()) {
      LOG(WARNING) << "line:" << line << " exist key or value empty";
      continue;
    }
    key_values[infos[0]] = infos[1];
  }

  // last times
  if (key_values.size() > 0) {
    int64_t begin_time_us = butil::gettimeofday_us();
    noah::common::Status status = cluster.Set(FLAGS_table_name, key_values, FLAGS_ttl);
    int64_t expired_us = butil::gettimeofday_us() - begin_time_us;
    if (noah::common::Status::kOk == status) {
      LOG(INFO) << "succ to" << FLAGS_table_name << " size:" << key_values.size() << " cost:" << expired_us << " us";
    } else {
      LOG(ERROR) << "failed to " << FLAGS_table_name << " status:" << status;
    }
    key_values.clear();
  }
  return 0;
}


