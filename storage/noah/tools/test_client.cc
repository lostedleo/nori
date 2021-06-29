// Author: Zhenwei Zhu losted.leo@gmail.com

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
#include "test_client/cluster.h"

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_string(meta_group, "MetaServer", "MetaServer group name");
DEFINE_string(meta_conf, "", "MetaServer configuration");
DEFINE_string(meta_test_conf, "", "MetaServer configuration");
DEFINE_string(table_name, "", "Table name");
DEFINE_int32(partition_num, 1, "Table Partition Number");
DEFINE_int32(duplicate_num, 2, "Table Duplicate Number");
DEFINE_int64(capacity, 0, "Table capacity size");
DEFINE_string(data_file, "", "File to read data");
DEFINE_int64(offset, 0, "Offset of lines to read file");
DEFINE_int32(size, 1000*1000, "Size of key_values");
DEFINE_int32(batch_size, 100, "Batch request size");
DEFINE_int32(ttl, -1, "Set value expired time in second");
DEFINE_int32(type, 0, "Operation type is set or get or list");

void DoClient(){
  noah::client::Options options(FLAGS_meta_group, FLAGS_meta_conf);
  noah::client::Cluster cluster(options);
  if (!cluster.Init()) {
    LOG(ERROR) << "Init cluster error";
    return;
  }


  std::cout << "input tablename " << std::endl;
  std::string table_name ;
  std::cin >> table_name;
  noah::common::Status status = cluster.CreateTable(table_name, FLAGS_partition_num,
                                                    FLAGS_duplicate_num, FLAGS_capacity);
  if (noah::common::Status::kOk != status &&
      noah::common::Status::kExist != status) {
    LOG(ERROR) << "Create table " << table_name << " error " << status;
    return;
  }
          KeyValueMap res;
          std::cout << "input keys " << std::endl;
          std::vector<std::string> keys_;
          KeyValueMap key_values;
          key_values.clear();

          for(int i = 0; i < 3; i++)
          {
            std::string key;
            std::cout << "key = " << std::endl;
            std::cin >> key;

            std::string value;
            std::cout << "value = " << std::endl;
            std::cin >> value;
            keys_.push_back(key);
            key_values[key] = value;
          }

          status = cluster.Set(table_name, key_values, -1);
          if (noah::common::Status::kOk == status) {
            LOG(INFO) << "client get keys success!";
            for(auto key : res){
              LOG(INFO) << "client get key " << key.first << " value " << key.second;
            }
          }

}

void DoTestClient(){
  noah::test_client::Options options_(FLAGS_meta_group, FLAGS_meta_test_conf);
  noah::test_client::Cluster cluster(options_);
  if (!cluster.Init()) {
    LOG(ERROR) << "Init test_cluster error";
    return ;
  }

  std::cout << "input tablename " << std::endl;
  std::string table_name ;
  std::cin >> table_name;
  noah::common::Status status = cluster.CreateTable(table_name, FLAGS_partition_num,
                                                    FLAGS_duplicate_num, FLAGS_capacity);
  if (noah::common::Status::kOk != status &&
      noah::common::Status::kExist != status) {
    LOG(ERROR) << "Test_client Create table " << table_name << " error " << status;
    return ;
  }

          KeyValueMap res;
          std::cout << "input keys " << std::endl;
          std::vector<std::string> keys_;
          KeyValueMap key_values;
          key_values.clear();

          for(int i = 0; i < 3; i++)
          {
            std::string key;
            std::cout << "key = " << std::endl;
            std::cin >> key;

            std::string value;
            std::cout << "value = " << std::endl;
            std::cin >> value;
            keys_.push_back(key);
            key_values[key] = value;
          }

          status = cluster.Set(table_name, key_values, -1);
          if (noah::common::Status::kOk == status) {
            LOG(INFO) << "test_client get keys success!";
            for(auto key : res){
              LOG(INFO) << "test_client get key " << key.first << " value " << key.second;
            }
          }

}
int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);


  DoClient();
  DoTestClient();
  return 0;
}


