// Author: Zhenwei Zhu losted.leo@gmail.com

#include <sstream>
#include <unistd.h>

#include <gflags/gflags.h>

#include "bthread/bthread.h"
#include "butil/logging.h"
#include "butil/time.h"
#include "butil/base64.h"
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

std::atomic<int> g_index;
std::atomic<int> g_falished;
std::atomic<size_t> g_size;

bvar::LatencyRecorder g_latency_recorder("import");
bvar::Adder<int> g_error_count("error_count");

struct TaskInfo {
  TaskInfo () : data(NULL) , length(0) {
  }

  const char* data;
  size_t      length;
  noah::client::Cluster* cluster;
};

static void* Import(void* arg) {
  TaskInfo* task_info = static_cast<TaskInfo*>(arg);
  const char* data = task_info->data;
  noah::client::Cluster* cluster = task_info->cluster;
  int index = g_index++;
  size_t start = (task_info->length / FLAGS_thread_num) * index;
  size_t end = 0;
  if (index == FLAGS_thread_num - 1) {
    end = task_info->length;
  } else {
    end = (task_info->length / FLAGS_thread_num) * (index + 1);
  }
  // if index is not 0, then find first \n
  size_t temp_start = start;
  if (index != 0) {
    while (data[start++] != '\n') {
    }
  }
  char buf[4096];
  int32_t buf_index = 0;
  size_t read_size = 0;
  std::map<std::string, KeyValueMap> my_map;
  std::vector<std::string> infos;
  int64_t begin_time_us = 0, expired_us = 0;
  std::string table_name, value;
  while (!brpc::IsAskedToQuit() && start < end) {
    buf[buf_index++] = data[start];
    if (data[start] == '\n') {
      buf[buf_index - 1] = '\0';
      read_size += buf_index;
      butil::SplitString(std::string(buf, strlen(buf)), ',', &infos);
      // LOG(INFO) << "infos.size() = " << infos.size() << " [" << buf << "] strlen(buf) = " << strlen(buf) << " buf_index = " << buf_index;
      // if (infos.size() >= 2) {
      //   LOG(INFO) << "0.sz = " << infos[0].size() << " 1.sz = " << infos[1].size() << " 2.sz = " << infos[2].size();
      //   LOG(INFO) << infos[0];
      // }
      buf_index = 0;
      
      if (infos.size() >= 2) {
          // table_name,dev_id,xxdata
        table_name = infos[0];
        butil::Base64Decode(infos[2], &value);
        my_map[table_name][infos[1]] = value;
        // LOG(ERROR) << "key = " << infos[1] << " value = " << infos[2];

        if ((int32_t)my_map[table_name].size() >= FLAGS_batch_size) {
          begin_time_us = butil::gettimeofday_us();
          // noah::common::Status status = cluster->Set(FLAGS_table_name, key_values, FLAGS_ttl);
          noah::common::Status status = cluster->Set(table_name, my_map[table_name], FLAGS_ttl);
          expired_us = butil::gettimeofday_us() - begin_time_us;
          if (noah::common::Status::kOk == status) {
            g_latency_recorder << expired_us;
          } else {
            LOG(ERROR) << "Set error " << status;
            g_error_count << 1;
            bthread_usleep(50000);
          }
          // key_values.clear();
          my_map[table_name].clear();
        }
      } else {
        LOG(WARNING) << "Error value " << buf;
      }
    }
    start++;
  }
  // find last end of line \n
  bool has_new = false;
  while (start < task_info->length && data[start] != '\n') {
    buf[buf_index++] = data[start++];
    has_new = true;
  }
  if (has_new) {
    buf[buf_index] = '\0';
    read_size += buf_index + 1;
    butil::SplitString(std::string(buf, strlen(buf)), ',', &infos);
    table_name = infos[0];
    if (infos.size() >= 2) {
      butil::Base64Decode(infos[2], &value);
      my_map[table_name][infos[1]] = value;
    }
  }
  for (const auto& it : my_map) {
    if (it.second.size()) {
      begin_time_us = butil::gettimeofday_us();
      // noah::common::Status status = cluster->Set(FLAGS_table_name, key_values, FLAGS_ttl);
      noah::common::Status status = cluster->Set(it.first, it.second, FLAGS_ttl);
      expired_us = butil::gettimeofday_us() - begin_time_us;
      if (noah::common::Status::kOk != status) {
        LOG(ERROR) << "Set error " << status;
        g_error_count << 1;
      }
      // key_values.clear();
    }
  }
  g_size += read_size;
  VLOG(10) << "id " << index << " start " << temp_start << " end " << end << " size "
    << end - temp_start << " read_size " << read_size;

  g_falished++;
  return NULL;
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  noah::client::Options options(FLAGS_meta_group, FLAGS_meta_conf);
  noah::client::Cluster cluster(options);
  if (!cluster.Init()) {
    LOG(ERROR) << "Init cluster error";
    return -1;
  }

  // std::string table_name = FLAGS_table_name;
  // noah::common::Status status = cluster.CreateTable(table_name, FLAGS_partition_num,
  //                                                   FLAGS_duplicate_num, FLAGS_capacity);
  // if (noah::common::Status::kOk != status &&
  //     noah::common::Status::kExist != status) {
  //   LOG(ERROR) << "Create table " << table_name << " error " << status;
  //   return -2;
  // }

  if (FLAGS_data_file.empty()) {
    LOG(ERROR) << "Please input file_name";
    return -1;
  }

  int64_t start = butil::gettimeofday_us();
  butil::FilePath file_path(FLAGS_data_file);
  butil::MemoryMappedFile mm_file;
  if (!mm_file.Initialize(file_path)) {
    LOG(ERROR) << "Initialize mmap file error";
    return -1;
  }

  TaskInfo task_info;
  task_info.data = (const char*)mm_file.data();
  task_info.length = mm_file.length();
  task_info.cluster = &cluster;
  std::vector<pthread_t> pids;
  pids.resize(FLAGS_thread_num);
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    if (pthread_create(&pids[i], NULL, Import, &task_info)) {
      LOG(ERROR) << "Failed to create pthread";
      return -1;
    }
  }

  while (!brpc::IsAskedToQuit() && g_falished < FLAGS_thread_num) {
    sleep(1);
    LOG(INFO) << "Sending Request at qps=" << g_latency_recorder.qps(1) * FLAGS_batch_size
              << " latency=" << g_latency_recorder.latency(1);
  }

  LOG(INFO) << "Import is going to quit";
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    pthread_join(pids[i], NULL);
  }

  int64_t expired = butil::gettimeofday_us() - start;
  LOG(INFO) << "size " << g_size << " file length " << mm_file.length() << " cost "
            << expired << " us";

  return 0;
}


