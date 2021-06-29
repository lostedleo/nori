// Author: Zhenwei Zhu losted.leo@gmail.com

#include <memory>

#include <gflags/gflags.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "butil/logging.h"
#include "butil/strings/string_split.h"

#include "client/cluster.h"
#include "common/entity.h"
#include "meta/meta.pb.h"

#include "tools/noah_client.h"
#include <unistd.h>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>

DEFINE_string(meta_group, "MetaServer", "MetaServer group name");
DEFINE_string(meta_conf, "", "MetaServer configuration");
DEFINE_int32(list_size, 10000, "List table key values in every batch");
DEFINE_int32(max_print_size, 100, "List table key values max size to print");
DEFINE_int32(timeout_ms, 3000, "Request timeout in ms");

// For freeing the memory returned by readline().
struct Freer {
  void operator()(char* mem) {
    free(mem);
  }
};

void usage() {
  LOG(INFO) << "Usage:  info {Command info is to look noah group status} \n";
  LOG(INFO) << "\tcreate_table table_name [partition_num duplicate_num capacity]";
  LOG(INFO) << " {Command create_table is create table}\n";
  LOG(INFO) << "\ttable_info table_name {Command table_info is look up table's";
  LOG(INFO) << " info}\n";
  LOG(INFO) << "\tset table_name key value [ttl] {Commnad set is set table key value}\n";
  LOG(INFO) << "\tget table_name key {Command get is get table key value}\n";
  LOG(INFO) << "\tdel table_name key {Command del is detele table key value}\n";
  LOG(INFO) << "\tdrop table_name {Command drop is drop table}\n";
  LOG(INFO) << "\tlist table_name offset size {Command list is list table key value}\n";
  return;
}

// The getc for readline. The default getc retries reading when meeting
// EINTR, which is not what we want.
void ProcessCommand(const std::string& command, noah::client::NoahClient* client, std::string &res) {
  std::vector<std::string> args;
  butil::SplitString(command, ' ', &args);
  if (args.empty()) {
    return;
  }
  if (args[0] == "info") {
    noah::common::MetaInfo* info = NULL;
    if (!client->Info(&info)) {
      LOG(INFO) << "Get info error\n";
      return;
    }
    noah::meta::MetaNodes meta_nodes;
    if (!client->GetMetaNodes(&meta_nodes)) {
      LOG(INFO) << "GetMetaNodes error\n";
      return;
    }
    LOG(INFO) << "Tables info:\n";
    for (const auto& value : info->table_info) {
      LOG(INFO) << " -table name: " << value.first << " version: " << value.second.version()
                << " partition_num: " << value.second.partition_num() << " capacity: "
                << value.second.capacity() << std::endl;
    }

    LOG(INFO) << "Version: " << info->version.DebugDump() << std::endl;
    LOG(INFO) << "Meta Servers:\n";
    LOG(INFO) << "leader: " << noah::common::Node(meta_nodes.leader()).ToString()
              << std::endl;
    if (meta_nodes.followers_size()) {
      LOG(INFO) << "followers:";
      int size = meta_nodes.followers_size();
      for (int i = 0; i < size; ++i) {
        LOG(INFO) << " " << noah::common::Node(meta_nodes.followers(i)).ToString();
      }
      LOG(INFO) << std::endl;
    }

    LOG(INFO) << "Nodes info:\n";
    for (const auto& value : info->nodes) {
      LOG(INFO) << value.DebugDump() << std::endl;
    }
  } else if (args[0] == "create_table") {
    if (args.size() < 2) {
      return usage();
    }
    int32_t partition_num = 1;
    int32_t duplicate_num = 2;
    int64_t capacity = 0;
    if (args.size() >= 3) {
      partition_num = atoi(args[2].c_str());
    }
    if (args.size() >= 4) {
      duplicate_num = atoi(args[3].c_str());
    }
    if (args.size() >= 5) {
      capacity = strtoll(args[4].c_str(), NULL, 10);
    }
    noah::common::Status status =
      client->cluster()->CreateTable(args[1], partition_num,
                                     duplicate_num, capacity);
    if (noah::common::Status::kOk == status) {
      LOG(INFO) << "Craete table " << args[1] << " success\n";
    } else if (noah::common::kExist == status) {
      LOG(INFO) << "Table " << args[1] << " exist\n";
    } else {
      LOG(INFO) << "Craete table " << args[1] << " error\n";
    }
  } else if (args[0] == "table_info") {
    if (args.size() < 2) {
      //LOG(INFO) << "talbe_info should have table_name" << std::endl;
      res +=  "talbe_info should have table_name\t";
      return;
    }
    noah::common::Table table;
    bool exist = client->FindTable(args[1], &table);
    if (!exist) {
      //LOG(INFO) << "Table " << args[1] << " not exist" << std::endl;
      res +=  args[1] + "nodata\t";
      return;
    }
    // LOG(INFO) << table.DebugDump();
    noah::client::Cluster* cluster = client->cluster();
    std::vector<int32_t> offsets;
    noah::common::Status status = cluster->ListTableInfo(args[1], &offsets);
    if (noah::common::Status::kOk != status) {

      //LOG(INFO) << "ListTableInfo error status: " << status << std::endl;
      res += "List_status:"+ std::to_string(status) +"\t";
      return;
    }
    res += std::to_string(offsets.back()) + "\t";
    //LOG(INFO) << " -total size: " << offsets.back() << std::endl;
//    for (size_t i = 0; i < offsets.size(); ++i) {
//      LOG(INFO) << " -" << i << " size: "
//                << (i == 0 ? offsets[i] : offsets[i] - offsets[i - 1]) << std::endl;
//    }
  } else if (args[0] == "set") {
    if (args.size() < 4) {
      return usage();
    }
    int32_t ttl = -1;
    if (args.size() >= 5) {
      ttl = atoi(args[4].c_str());
    }
    bool ret = client->Set(args[1], args[2], args[3], ttl);
    if (ret) {
      LOG(INFO) << "Set table: " << args[1] << " key: " << args[2] << " value: "
                << args[3] << " success" << std::endl;
    } else {
      LOG(INFO) << "Set table: " << args[1] << " key: " << args[2] << " value: "
                << args[3] << " error" << std::endl;
    }
  } else if (args[0] == "get") {
    if (args.size() < 3) {
      return usage();
    }
    std::string value;
    bool ret = client->Get(args[1], args[2], &value);
    if (ret) {
      if (value.empty()) {
        LOG(INFO) << "Get table: " << args[1] << " key: " << args[2] << " not exist\n";
      } else {
        LOG(INFO) << "Get table: " << args[1] << " key: " << args[2] << " value: "
                  << value << std::endl;
      }
    } else {
      LOG(INFO) << "Get table: " << args[1] << " key: " << args[2] << " error"
                << std::endl;
    }
  } else if (args[0] == "del") {
    if (args.size() < 3) {
      return usage();
    }
    std::string value;
    bool ret = client->Del(args[1], args[2]);
    if (ret) {
      LOG(INFO) << "Del table: " << args[1] << " key: " << args[2] << " success\n";
    } else {
      LOG(INFO) << "Del table: " << args[1] << " key: " << args[2] << " error\n";
    }
  } else if (args[0] == "drop") {
    if (args.size() < 2) {
      return usage();
    }
    bool ret = client->DropTable(args[1]);
    if (ret) {
      LOG(INFO) << "Drop table: " << args[1] << " success\n";
    } else {
      LOG(INFO) << "Drop table: " << args[1] << " error\n";
    }
  } else if (args[0] == "list") {
    if (args.size() != 4) {
      return usage();
    }
    bool end = false;
    int32_t total_size = atoi(args[3].c_str());
    int32_t offset = atoi(args[2].c_str());
    int32_t size = 0;
    int32_t request_size = 0;
    noah::common::Status status;
    KeyValueMap key_values;
    std::unique_ptr<noah::client::Iterator> iterator(client->NewIterator(args[1]));
    while (!end && size < total_size) {
      request_size = total_size - size;
      if (request_size == 0) {
        break;
      }
      if (request_size >= FLAGS_list_size) {
        request_size = FLAGS_list_size;
      }
      status = iterator->List(offset + size, request_size, &key_values, &end);
      if (noah::common::Status::kOk != status) {
        LOG(INFO) << "List table: " << args[1] << " error\n";
        break;
      }
      LOG(INFO) << "List table: " << args[1] << " offset: " << offset + size
                << " size: " << request_size << " real_size:" << key_values.size()
                << " end: " << end << "\n";
      if ((int32_t)key_values.size() < FLAGS_max_print_size) {
        for (const auto& value : key_values) {
          LOG(INFO) << "key: " << value.first << " value: "  << value.second << "\n";
        }
      }
      size += request_size;
    }
  } else {
    res += "Not support command: " + command  + "\t";
  }
  return;
}

std::vector<std::string> ip{
        "10.165.19.220",
        "100.107.24.84",
        "100.107.24.20",
        "100.107.24.142",
        "100.107.24.141",
        "100.107.23.208",
        "100.107.20.18",
        "100.107.19.143",
        "100.107.19.142",
        "100.107.24.85",
        "100.65.28.210",
        "100.94.7.96",
        "100.94.10.211",
        "100.66.0.97",
        "100.66.0.96",
        "100.65.29.144",
        "100.65.29.132",
        "100.65.29.131",
        "100.65.28.77",
        "100.107.18.38",
        "10.49.105.229",
        "10.165.22.4",
        "10.165.22.3",
        "10.165.22.26",
        "10.165.21.88",
        "10.165.21.86",
        "10.165.21.84",
        "10.165.21.36",
        "10.165.20.30",
        "10.165.22.6",
        "10.165.22.68",
        "10.175.122.99",
        "10.175.122.98",
        "10.175.121.74",
        "10.175.121.68",
        "10.165.22.73",
        "10.165.22.72",
        "10.165.22.71",
        "10.165.22.7",
        "100.94.9.14",
};

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

//  noah::client::Options options(FLAGS_meta_group, FLAGS_meta_conf);
//  options.request_timeout_ms = FLAGS_timeout_ms;
//  noah::client::Cluster cluster(options);
//  if (!cluster.Init()) {
//    LOG(ERROR) << "Init cluster error";
//    return -1;
//  }
//  noah::client::NoahClient noah_client(&cluster);

  butil::ip_t ip;
  ip = butil::my_ip();
  LOG(INFO) << "ip == " << butil::ip2str(ip) ;
  return 0;

}
