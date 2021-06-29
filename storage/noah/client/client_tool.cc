// Author: Zhenwei Zhu losted.leo@gmail.com

#include <sstream>

#include <gflags/gflags.h>

#include "bthread/bthread.h"
#include "butil/logging.h"
#include "butil/time.h"
#include "bvar/bvar.h"
#include "butil/rand_util.h"
#include "butil/base64.h"

#include "client/cluster.h"
#include "client/file_read.h"

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_bthread, true, "Use bthread to send requests");
DEFINE_string(meta_group, "MetaServer", "MetaServer group name");
DEFINE_string(meta_conf, "", "MetaServer configuration");
DEFINE_string(table_name, "", "Table name");
DEFINE_int32(partition_num, 1, "Table Partition Number");
DEFINE_int32(duplicate_num, 2, "Table Duplicate Number");
DEFINE_int64(capacity, 10 * 1000 * 1000, "Table capacity size");
DEFINE_string(data_file, "", "File to read data");
DEFINE_string(save_file, "", "File to save key_vlues");
DEFINE_int64(offset, 0, "Offset of lines to read file");
DEFINE_int32(size, 1000*1000, "Size of key_values");
DEFINE_int32(key_size, 16, "Bytes of key");
DEFINE_int32(value_size, 16, "Bytes of value");
DEFINE_int32(batch_size, 100, "Batch request size");
DEFINE_int32(ttl, -1, "Set value expired time in second");
DEFINE_int32(type, 0, "Operation type is set or get or list");

std::vector<std::string>* g_keys = NULL;
std::vector<std::string>* g_values = NULL;
std::atomic<int> g_index;

bvar::LatencyRecorder g_latency_recorder("client");
bvar::Adder<int> g_error_count("client_error_count");

static void* Set(void* arg) {
  noah::client::Cluster* cluster = static_cast<noah::client::Cluster*>(arg);
  int index = g_index++;
  uint32_t start = (FLAGS_size / FLAGS_thread_num) * index;
  KeyValueMap key_values;
  int64_t begin_time_us = 0, expired_us = 0;
  while (!brpc::IsAskedToQuit()) {
    key_values.clear();
    for (int i = 0; i < FLAGS_batch_size; ++i) {
      key_values[(*g_keys)[start % FLAGS_size]] = (*g_values)[start % FLAGS_size];
      start++;
    }
    begin_time_us = butil::gettimeofday_us();
    noah::common::Status status = cluster->Set(FLAGS_table_name, key_values, FLAGS_ttl);
    expired_us = butil::gettimeofday_us() - begin_time_us;
    if (noah::common::Status::kOk == status) {
      g_latency_recorder << expired_us;
    } else {
      LOG(ERROR) << "Set error " << status;
      g_error_count << 1;
      bthread_usleep(50000);
    }
  }
  return NULL;
}

static void* Get(void* arg) {
  noah::client::Cluster* cluster = static_cast<noah::client::Cluster*>(arg);
  int index = g_index++;
  uint32_t start = (FLAGS_size / FLAGS_thread_num) * index;
  StringVector keys;
  KeyValueMap key_values;
  int64_t begin_time_us = 0, expired_us = 0;
  while (!brpc::IsAskedToQuit()) {
    keys.clear();
    key_values.clear();
    for (int i = 0; i < FLAGS_batch_size; ++i) {
      keys.push_back((*g_keys)[start % FLAGS_size]);
      start++;
    }
    begin_time_us = butil::gettimeofday_us();
    noah::common::Status status = cluster->Get(FLAGS_table_name, keys, &key_values);
    expired_us = butil::gettimeofday_us() - begin_time_us;
    if (noah::common::Status::kOk == status) {
      g_latency_recorder << expired_us;
    } else {
      LOG(ERROR) << "Get error " << status;
      g_error_count << 1;
      bthread_usleep(50000);
    }
  }
  return NULL;
}

static void* List(void* arg) {
  noah::client::Cluster* cluster = static_cast<noah::client::Cluster*>(arg);
  KeyValueMap key_values;
  int64_t begin_time_us = 0, expired_us = 0;
  bool end = false;
  int32_t offset = g_index++;
  int32_t size = 0;
  std::unique_ptr<noah::client::Iterator> iterator(cluster->NewIterator(FLAGS_table_name));
  while (!brpc::IsAskedToQuit()) {
    begin_time_us = butil::gettimeofday_us();
    noah::common::Status status = iterator->List(offset + size, FLAGS_batch_size,
                                                 &key_values, &end);
    expired_us = butil::gettimeofday_us() - begin_time_us;
    if (noah::common::Status::kOk == status) {
      g_latency_recorder << expired_us;
      size += FLAGS_batch_size;
      if (end) {
        size = 0;
        offset = 0;
      }
    } else {
      LOG(ERROR) << "List error " << status;
      g_error_count << 1;
      bthread_usleep(50000);
    }
  }
  return NULL;
}

bool get_key_values() {
  g_keys = new std::vector<std::string>();
  g_values = new std::vector<std::string>();
  if (FLAGS_data_file.empty()) {
    std::string key, base64_key, value, base64_value;
    for (int i = 0; i < FLAGS_size; ++i) {
      key = butil::RandBytesAsString(FLAGS_key_size);
      value = butil::RandBytesAsString(FLAGS_value_size);
      butil::Base64Encode(key, &base64_key);
      butil::Base64Encode(value, &base64_value);
      g_keys->push_back(base64_key);
      g_values->push_back(base64_value);
    }
    if (!FLAGS_save_file.empty()) {
      butil::ScopedFILE save_file(FLAGS_save_file.c_str(), "w+");
      FILE* fd = save_file.get();
      if (!fd) {
        LOG(ERROR) << "Failed to open file " << FLAGS_save_file;
        return false;
      }
      for (int i = 0; i < FLAGS_size; ++i) {
        fputs((*g_keys)[i].c_str(), fd);
        fputs(",", fd);
        fputs((*g_values)[i].c_str(), fd);
        fputs("\n", fd);
      }
    }
    return true;
  } else {
    noah::client::FileRead file_read(FLAGS_data_file.c_str());
    int size = file_read.Read(FLAGS_offset, FLAGS_size, g_keys, g_values);
    if (size != FLAGS_size) {
      LOG(INFO) << "get key_values error size:" << size << " expect size:"
        << FLAGS_size;
      FLAGS_size = size;
    }
  }
  return true;
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  noah::client::Options options(FLAGS_meta_group, FLAGS_meta_conf);
  options.auto_partition_num = FLAGS_partition_num;
  options.auto_duplicate_num = FLAGS_duplicate_num;
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

  if (!get_key_values()) {
    LOG(ERROR) << "Get key_values error";
    return -3;
  }
  void *(*func_ptr)(void*);
  if (0 == FLAGS_type) {
    func_ptr = Set;
  } else if (1 == FLAGS_type) {
    func_ptr = Get;
  } else {
    func_ptr = List;
  }
  std::vector<bthread_t> tids;
  tids.resize(FLAGS_thread_num);
#if defined(OS_LINUX)
  if (!FLAGS_use_bthread) {
    for (int i = 0; i < FLAGS_thread_num; ++i) {
      if (pthread_create(&tids[i], NULL, func_ptr, &cluster) != 0) {
        LOG(ERROR) << "Failed to create pthread";
        return -1;
      }
    }
  } else {
    for (int i = 0; i < FLAGS_thread_num; ++i) {
      if (bthread_start_background(&tids[i], NULL, func_ptr, &cluster) != 0) {
        LOG(ERROR) << "Failed to create bthread";
        return -1;
      }
    }
  }
#elif defined(OS_MACOSX)
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    if (bthread_start_background(&tids[i], NULL, func_ptr, &cluster) != 0) {
      LOG(ERROR) << "Failed to create bthread";
      return -1;
    }
  }
#endif

  while (!brpc::IsAskedToQuit()) {
    sleep(1);
    LOG(INFO) << "Sending Request at qps=" << g_latency_recorder.qps(1) * FLAGS_batch_size
      << " latency=" << g_latency_recorder.latency(1);
  }

  LOG(INFO) << "Client is going to quit";
#if defined(OS_LINUX)
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    if (!FLAGS_use_bthread) {
      pthread_join(tids[i], NULL);
    } else {
      bthread_join(tids[i], NULL);
    }
  }
#elif defined(OS_MACOSX)
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    bthread_join(tids[i], NULL);
  }
#endif

  delete g_keys;
  delete g_values;

  return 0;
}


