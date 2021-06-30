// Author: Zhenwei Zhu losted.leo@gmail.com

#include <memory>
#include <signal.h>

#include <gflags/gflags.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "butil/logging.h"
#include "butil/strings/string_split.h"

#include "client/cluster.h"
#include "common/entity.h"
#include "meta/meta.pb.h"

#include "tools/noah_client.h"

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

static void dummy_handler(int) {}

void usage() {
  std::cout << "Usage:  info {Command info is to look noah group status} \n";
  std::cout << "\tcreate_table table_name [partition_num duplicate_num capacity]";
  std::cout << " {Command create_table is create table}\n";
  std::cout << "\ttable_info table_name {Command table_info is look up table's";
  std::cout << " info}\n";
  std::cout << "\tset table_name key value [ttl] {Commnad set is set table key value}\n";
  std::cout << "\tget table_name key {Command get is get table key value}\n";
  std::cout << "\tdel table_name key {Command del is detele table key value}\n";
  std::cout << "\tdrop table_name {Command drop is drop table}\n";
  std::cout << "\tlist table_name offset size {Command list is list table key value}\n";
  return;
}

// The getc for readline. The default getc retries reading when meeting
// EINTR, which is not what we want.
static bool g_canceled = false;
static int cli_getc(FILE *stream) {
  int c = getc(stream);
  if (c == EOF && errno == EINTR) {
    g_canceled = true;
    return '\n';
  }
  return c;
}

void ProcessCommand(const std::string& command, noah::client::NoahClient* client) {
  std::vector<std::string> args;
  butil::SplitString(command, ' ', &args);
  if (args.empty()) {
    return;
  }
  if (args[0] == "info") {
    noah::common::MetaInfo* info = NULL;
    if (!client->Info(&info)) {
      std::cout << "Get info error\n";
      return;
    }
    noah::meta::MetaNodes meta_nodes;
    if (!client->GetMetaNodes(&meta_nodes)) {
      std::cout << "GetMetaNodes error\n";
      return;
    }
    std::cout << "Tables info:\n";
    for (const auto& value : info->table_info) {
      std::cout << " -table name: " << value.first << " version: " << value.second.version()
                << " partition_num: " << value.second.partition_num() << " capacity: "
                << value.second.capacity() << std::endl;
    }

    std::cout << "Version: " << info->version.DebugDump() << std::endl;
    std::cout << "Meta Servers:\n";
    std::cout << "leader: " << noah::common::Node(meta_nodes.leader()).ToString()
              << std::endl;
    if (meta_nodes.followers_size()) {
      std::cout << "followers:";
      int size = meta_nodes.followers_size();
      for (int i = 0; i < size; ++i) {
        std::cout << " " << noah::common::Node(meta_nodes.followers(i)).ToString();
      }
      std::cout << std::endl;
    }

    std::cout << "Nodes info:\n";
    for (const auto& value : info->nodes) {
      std::cout << value.DebugDump() << std::endl;
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
      std::cout << "Craete table " << args[1] << " success\n";
    } else if (noah::common::kExist == status) {
      std::cout << "Table " << args[1] << " exist\n";
    } else {
      std::cout << "Craete table " << args[1] << " error\n";
    }
  } else if (args[0] == "table_info") {
    if (args.size() < 2) {
      std::cout << "talbe_info should have table_name" << std::endl;
      return;
    }
    noah::common::Table table;
    bool exist = client->FindTable(args[1], &table);
    if (!exist) {
      std::cout << "Table " << args[1] << " not exist" << std::endl;
      return;
    }
    std::cout << table.DebugDump();
    noah::client::Cluster* cluster = client->cluster();
    std::vector<int32_t> offsets;
    noah::common::Status status = cluster->ListTableInfo(args[1], &offsets);
    if (noah::common::Status::kOk != status) {
      std::cout << "ListTableInfo error status: " << status << std::endl;
      return;
    }
    std::cout << " -total size: " << offsets.back() << std::endl;
    for (size_t i = 0; i < offsets.size(); ++i) {
      std::cout << " -" << i << " size: "
                << (i == 0 ? offsets[i] : offsets[i] - offsets[i - 1]) << std::endl;
    }
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
      std::cout << "Set table: " << args[1] << " key: " << args[2] << " value: "
                << args[3] << " success" << std::endl;
    } else {
      std::cout << "Set table: " << args[1] << " key: " << args[2] << " value: "
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
        std::cout << "Get table: " << args[1] << " key: " << args[2] << " not exist\n";
      } else {
        std::cout << "Get table: " << args[1] << " key: " << args[2] << " value: "
                  << value << std::endl;
      }
    } else {
      std::cout << "Get table: " << args[1] << " key: " << args[2] << " error"
                << std::endl;
    }
  } else if (args[0] == "del") {
    if (args.size() < 3) {
      return usage();
    }
    std::string value;
    bool ret = client->Del(args[1], args[2]);
    if (ret) {
      std::cout << "Del table: " << args[1] << " key: " << args[2] << " success\n";
    } else {
      std::cout << "Del table: " << args[1] << " key: " << args[2] << " error\n";
    }
  } else if (args[0] == "drop") {
    if (args.size() < 2) {
      return usage();
    }
    bool ret = client->DropTable(args[1]);
    if (ret) {
      std::cout << "Drop table: " << args[1] << " success\n";
    } else {
      std::cout << "Drop table: " << args[1] << " error\n";
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
        std::cout << "List table: " << args[1] << " error\n";
        break;
      }
      std::cout << "List table: " << args[1] << " offset: " << offset + size
                << " size: " << request_size << " real_size:" << key_values.size()
                << " end: " << end << "\n";
      if ((int32_t)key_values.size() < FLAGS_max_print_size) {
        for (const auto& value : key_values) {
          std::cout << "key: " << value.first << " value: "  << value.second << "\n";
        }
      }
      size += request_size;
    }
  } else {
    std::cout << "Not support command: " << command << std::endl;
  }
  return;
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  noah::client::Options options(FLAGS_meta_group, FLAGS_meta_conf);
  options.request_timeout_ms = FLAGS_timeout_ms;
  noah::client::Cluster cluster(options);
  if (!cluster.Init()) {
    LOG(ERROR) << "Init cluster error";
    return -1;
  }
  noah::client::NoahClient noah_client(&cluster);

  if (argc <=1) {
    // We need this dummy signal hander to interrupt getc (and returning
    // EINTR), SIG_IGN did not work.
    signal(SIGINT, dummy_handler);

    // Hook getc of readline.
    rl_getc_function = cli_getc;

    for (;;) {
      char prompt[64];
      snprintf(prompt, sizeof(prompt), "noah >");
      std::unique_ptr<char, Freer> command(readline(prompt));
      if (command == NULL || *command == '\0') {
        if (g_canceled) {
          // No input after the prompt and user pressed Ctrl-C,
          // quit the CLI.
          return 0;
        }
        continue;
      }
      if (g_canceled) {
        // User entered sth. and pressed Ctrl-C, start a new prompt.
        g_canceled = false;
        continue;
      }
      // Add user's command to history so that it's browse-able by
      // UP-key and search-able by Ctrl-R.
      add_history(command.get());

      if (!strcmp(command.get(), "help")) {
        usage();
        continue;
      }
      if (!strcmp(command.get(), "quit")) {
        return 0;
      }
      ProcessCommand(command.get(), &noah_client);
    }
  } else {
    std::string command;
    command.reserve(argc * 16);
    for (int i = 1; i < argc; ++i) {
      if (i != 1) {
        command.push_back(' ');
      }
      command.append(argv[i]);
    }
    ProcessCommand(command, &noah_client);
  }

  return 0;
}
