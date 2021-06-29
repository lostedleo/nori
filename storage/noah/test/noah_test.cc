#include <string>
#include <thread>
#include <random>

#include <gtest/gtest.h>
#include "brpc/server.h"
#include "braft/raft.h"

#include "meta/meta_server.h"
#include "meta/meta_service.h"
#include "node/node_server.h"
#include "node/node_service.h"
#include "tools/noah_client.h"
#include "client/options.h"
#include "client/cluster.h"

namespace fLB {
extern bool FLAGS_auto_migrate;
}

namespace noah {
namespace client {

typedef std::vector<std::string> StringVector;
typedef std::unordered_map<std::string, std::string> KeyValueMap;
typedef std::unique_ptr<Iterator> IteratorUniPtr;

bool Contains(const StringVector& sv, const std::string& item) {
  return std::find(sv.begin(), sv.end(), item) != sv.end();
}

int RandomInt(int from, int to) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(from, to);
  return dist(mt);
}

template<typename Map>
std::vector<typename Map::key_type> Keys(const Map& map) {
  std::vector<typename Map::key_type> keys{};
  keys.reserve(map.size());
  for (const auto& kv : map) {
    keys.push_back(kv.first);
  }
  return keys;
}

template<typename Map>
void PrintMap(Map map) {  // For debugging only
  std::cout << "{" << std::endl;
  for (const auto& kv : map) {
    std::cout << "  \"" << kv.first << "\" : \"" << kv.second << "\""
              << std::endl;
  }
  std::cout << "}" << std::endl;
}

void Print() {}  // For debugging only

template<typename T, typename... Tail>
void Print(T head, Tail... tail) {  // For debugging only
  std::cout << head;
  Print(tail...);
}

template<typename... T>
void PrintInRed(T... args) {  // For debugging only
  std::cout << "\033[31m";
  Print(args...);
  std::cout << "\033[0m" << std::endl;
}

class MetaServerWrapper {
 public:
  explicit MetaServerWrapper(const std::string& data_path,
                             const std::string& meta_conf, int port)
    : data_path_(data_path),
      meta_conf_(meta_conf),
      port_(port) {
    Init();
  }

  void Init() {
    butil::ip_t ip;
    butil::str2ip(ip_.c_str(), &ip);
    addr_ = {ip, port_};

    auto* service = new noah::meta::MetaServiceImpl(&meta_server_);
    ASSERT_EQ(server_.AddService(service, brpc::SERVER_OWNS_SERVICE), 0);
    ASSERT_EQ(braft::add_service(&server_, port_), 0);
    options_.internal_port = -1;

    ASSERT_EQ(node_options_.initial_conf.parse_from(meta_conf_), 0);
    node_options_.election_timeout_ms = 1000;
    node_options_.fsm = &meta_server_;
    node_options_.node_owns_fsm = false;
    node_options_.snapshot_interval_s = 30;
    std::string prefix = "local://" + data_path_;
    node_options_.log_uri = prefix + "/log";
    node_options_.raft_meta_uri = prefix + "/raft_meta";
    node_options_.snapshot_uri = prefix + "/snapshot";
    node_options_.disable_cli = true;
  }

  void Start() {
    ASSERT_EQ(server_.Start(port_, &options_), 0);
    ASSERT_EQ(meta_server_.Start(addr_, node_options_), 0);
  }

  void Stop() {
    meta_server_.ShutDown();
    server_.Stop(0);
    meta_server_.Join();
    server_.Join();
  }

  std::string ip_ = "127.0.0.1";
  noah::meta::MetaServer meta_server_{};

  std::string data_path_;
  std::string meta_conf_;
  int port_;

  butil::EndPoint addr_;
  brpc::ServerOptions options_;
  braft::NodeOptions node_options_;
  brpc::Server server_;
};

class MetaServerCluster {
 public:
  MetaServerCluster(const std::string& base_data_path, int base_port,
                    int node_num)
    : base_data_path_(base_data_path),
      base_port_(base_port),
      node_num_(node_num) {
    for (int i = 0; i < node_num_; i++) {
      int port = base_port + i;
      meta_conf_ += "127.0.0.1:";
      meta_conf_ += std::to_string(port);
      meta_conf_ += ":0,";
    }

    for (int i = 0; i < node_num_; i++) {
      std::string data_path = base_data_path + "/" + std::to_string(i);
      int port = base_port + i;
      auto* p = new MetaServerWrapper(data_path, meta_conf_, port);
      meta_server_wrappers_.push_back(std::unique_ptr<MetaServerWrapper>(p));
    }
  }

  void Start(int index) {
    meta_server_wrappers_[index]->Start();
  }

  void StartAll() {
    for (auto& meta_server : meta_server_wrappers_) {
      meta_server->Start();
    }
  }

  void Stop(int index) {
    meta_server_wrappers_[index]->Stop();
  }

  void StopAll() {
    for (auto& meta_server : meta_server_wrappers_) {
      meta_server->Stop();
    }
  }

  int GetLeaderIndex() {
    for (std::size_t i = 0; i < meta_server_wrappers_.size(); i++) {
      if (meta_server_wrappers_[i]->meta_server_.IsLeader()) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  std::vector<int> GetFollowerIndexes() {
    std::vector<int> indexes{};
    for (std::size_t i = 0; i < meta_server_wrappers_.size(); i++) {
      if (!meta_server_wrappers_[i]->meta_server_.IsLeader()) {
        indexes.push_back(static_cast<int>(i));
      }
    }
    return indexes;
  }

  void WaitForLeader() {
    while (GetLeaderIndex() == -1) {
      usleep(100 * 1000);
    }
  }

  int StopLeader() {
    int leader_index = GetLeaderIndex();
    if (leader_index != -1) {
      Stop(leader_index);
    }
    return leader_index;
  }

  int StopRandomFollower() {
    auto follower_indexes = GetFollowerIndexes();
    if (follower_indexes.empty()) {
      return -1;
    }

    int target_index = RandomInt(
      0, static_cast<int>(follower_indexes.size()) - 1);
    Stop(follower_indexes[target_index]);
    return target_index;
  }

  std::string base_data_path_;
  int base_port_;
  int node_num_;

  std::string meta_conf_;
  std::vector<std::unique_ptr<MetaServerWrapper>> meta_server_wrappers_;
};

class NodeServerWrapper {
 public:
  NodeServerWrapper(const std::string& meta_conf, int port)
    : meta_conf_(meta_conf),
      port_(port) {
    Init();
  }

  void Init() {
    noah::node::NodeServerOptions node_options;
    node_options.meta_group = "MetaServer";
    node_options.meta_conf = meta_conf_;
    node_options.ip = ip_;
    node_options.port = port_;
    node_options.ping_interval_ms = 200;
    node_options.expired_interval_ms = 3600 * 1000;
    node_options.node_timeout_ms = 500;

    node_server_.reset(new noah::node::NodeServer(node_options));

    auto* node_service_impl = new noah::node::NodeServiceImpl(
      node_server_.get());
    ASSERT_EQ(
      server_.AddService(node_service_impl, brpc::SERVER_OWNS_SERVICE), 0);

    server_options_.idle_timeout_sec = -1;
    server_options_.max_concurrency = 0;
    server_options_.internal_port = -1;
  }

  void Start() {
    ASSERT_EQ(node_server_->Start(), 0);
    ASSERT_EQ(server_.Start(port_, &server_options_), 0);
  }

  void Stop() {
    node_server_->ShutDown();
    LOG(INFO) << "NodeServer shut down!";
    server_.Stop(0);
    server_.Join();
  }

  std::string ip_ = "127.0.0.1";

  std::string meta_conf_;
  int port_;

  brpc::ServerOptions server_options_;
  brpc::Server server_;
  std::unique_ptr<noah::node::NodeServer> node_server_;
};

class NodeServerCluster {
 public:
  NodeServerCluster(const std::string& meta_conf, int base_port, int node_num)
    : meta_conf_(meta_conf),
      base_port_(base_port),
      node_num_(node_num) {
    for (int i = 0; i < node_num_; i++) {
      auto* p = new NodeServerWrapper(meta_conf_, base_port_ + i);
      node_server_wrappers_.push_back(std::unique_ptr<NodeServerWrapper>(p));
    }
  }

  void Start(int index) {
    node_server_wrappers_[index]->Start();
  }

  void StartAll() {
    for (auto& node_server : node_server_wrappers_) {
      node_server->Start();
    }
  }

  void Stop(int index) {
    node_server_wrappers_[index]->Stop();
  }

  void StopAll() {
    for (auto& node_server : node_server_wrappers_) {
      node_server->Stop();
    }
  }

  int StopRandom() {
    int target_index = RandomInt(
      0, static_cast<int>(node_server_wrappers_.size()) - 1);
    node_server_wrappers_[target_index]->Stop();
    return target_index;
  }

  std::string meta_conf_;
  int base_port_;
  int node_num_;

  std::vector<std::unique_ptr<NodeServerWrapper>> node_server_wrappers_;
};

class ClientWrapper {
 public:
  explicit ClientWrapper(const std::string& meta_conf, int node_num)
    : meta_conf_(meta_conf), node_num_(node_num) {}

  void Init() {
    noah::client::Options options("MetaServer", meta_conf_);
    options.request_timeout_ms = 200;
    cluster_.reset(new noah::client::Cluster(options));
    ASSERT_TRUE(cluster_->Init());
  }

  void WaitForNodeServers() {
    while (true) {
      bool ok = UpdateTableInfo({}, true);
      if (ok && cluster_->meta_info_.nodes.size() == (std::size_t) node_num_) {
        break;
      }
      usleep(100 * 1000);
    }
  }

  bool UpdateTableInfo(const StringVector& tables, bool all) {
    return cluster_->UpdateTableInfo(tables, all);
  }

  Cluster::MetaInfo GetMetaInfo() {
    butil::ReaderAutoLock lock_r(&cluster_->meta_lock_);
    return cluster_->meta_info_;
  }

  StringVector GetTableNames() {
    UpdateTableInfo({}, true);
    auto meta_info = GetMetaInfo();
    StringVector table_names{};
    for (auto& table_info : meta_info.table_info) {
      table_names.push_back(table_info.first);
    }
    return table_names;
  }

  std::string meta_conf_;
  int node_num_;

  std::unique_ptr<noah::client::Cluster> cluster_;
};

class NoahTest : public ::testing::Test {
 public:
  void SetUp() override {
    fLB::FLAGS_auto_migrate = true;

    system(("rm -rf " + meta_base_data_path_).c_str());
    meta_server_cluster_.StartAll();
    meta_server_cluster_.WaitForLeader();

    node_server_cluster_.StartAll();

    client_wrapper_.Init();
    client_wrapper_.WaitForNodeServers();
  }

  void TearDown() override {
    node_server_cluster_.StopAll();

    meta_server_cluster_.StopAll();
    system(("rm -rf " + meta_base_data_path_).c_str());
  }

  bool RetryCheck(const std::function<bool()>& check, int max_retry_num,
                  int interval_ms = 1500) {
    for (int i = 0; i < max_retry_num; i++) {
      if (check()) {
        return true;
      }

      if (i != max_retry_num - 1) {
        std::cout << "Retry in " << interval_ms << " ms ..." << std::endl;
        usleep(1000 * interval_ms);
      }
    }
    return false;
  }

  const std::string meta_base_data_path_ = "/tmp/test_noah_meta";
  const int meta_base_port_ = 9000;
  const int meta_num_ = 5;
  const int node_base_port_ = 9100;
  const int node_num_ = 4;

  MetaServerCluster meta_server_cluster_{
    meta_base_data_path_, meta_base_port_, meta_num_};
  NodeServerCluster node_server_cluster_{
    meta_server_cluster_.meta_conf_, node_base_port_, node_num_};
  ClientWrapper client_wrapper_{meta_server_cluster_.meta_conf_, node_num_};
};

// ========= Test normal

TEST_F(NoahTest, CreateTable) {
  std::string table_name = "table_1";
  auto status = client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  ASSERT_EQ(status, noah::common::Status::kOk)
            << "Fail to create table " << table_name;
  ASSERT_EQ(client_wrapper_.GetTableNames(), StringVector{table_name});
}

TEST_F(NoahTest, SetAndGet) {
  std::string table_name = "table_1";
  auto status = client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 2);
  ASSERT_EQ(status, noah::common::Status::kOk)
            << "Fail to create table " << table_name;

  client_wrapper_.cluster_->Set(table_name, {{"A", "1000"}, {"B", "2000"}}, -1);
  client_wrapper_.cluster_->Set(table_name, {{"C", "3000"}, {"D", "4000"}}, 1);
  KeyValueMap key_values{};
  client_wrapper_.cluster_->Get(table_name, {"A", "B", "C", "D"}, &key_values);
  ASSERT_EQ(
    key_values,
    KeyValueMap({{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}}));

  usleep(1020 * 1000);

  key_values.clear();
  client_wrapper_.cluster_->Get(
    table_name, {"A", "B", "C", "D", "E"}, &key_values);
  ASSERT_EQ(key_values,
            KeyValueMap({{"A", "1000"}, {"B", "2000"}, {"C", ""}, {"D", ""},
                         {"E", ""}}));
}

TEST_F(NoahTest, List) {
  std::string table_name = "table_1";
  auto status = client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  ASSERT_EQ(status, noah::common::Status::kOk)
            << "Fail to create table " << table_name;

  client_wrapper_.cluster_->Set(
    table_name, {{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}},
    -1);
  IteratorUniPtr iterator{client_wrapper_.cluster_->NewIterator(table_name)};

  KeyValueMap key_values{};
  bool is_end = false;
  status = iterator->List(0, 5, &key_values, &is_end);
  ASSERT_EQ(status, noah::common::Status::kOk);
  ASSERT_EQ(
    key_values,
    KeyValueMap({{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}}));
  ASSERT_TRUE(is_end);

  key_values.clear();
  status = iterator->List(0, 3, &key_values, &is_end);
  ASSERT_EQ(status, noah::common::Status::kOk);
  ASSERT_EQ(key_values.size(), 3);
  ASSERT_FALSE(is_end);

  key_values.clear();
  status = iterator->List(3, 2, &key_values, &is_end);
  ASSERT_EQ(status, noah::common::Status::kOk);
  ASSERT_EQ(key_values.size(), 1);
  ASSERT_TRUE(is_end);
}

TEST_F(NoahTest, Del) {
  std::string table_name = "table_1";
  auto status = client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 2);
  ASSERT_EQ(status, noah::common::Status::kOk)
            << "Fail to create table " << table_name;

  client_wrapper_.cluster_->Set(table_name,
                                {{"A", "1"}, {"B", "2"}, {"C", "3"}}, -1);
  client_wrapper_.cluster_->Delete(table_name, {"A", "B"});
  KeyValueMap key_values{};
  client_wrapper_.cluster_->Get(table_name, {"A", "B", "C"}, &key_values);
  ASSERT_EQ(key_values, KeyValueMap({{"A", ""}, {"B", ""}, {"C", "3"}}));
}

TEST_F(NoahTest, DropTable) {
  std::string table_name = "table_1";
  auto status = client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 2);
  ASSERT_EQ(status, noah::common::Status::kOk)
            << "Fail to create table " << table_name;

  status = client_wrapper_.cluster_->DropTable({table_name});
  ASSERT_EQ(status, noah::common::Status::kOk);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector{});
}

// ========= Test MetaServer failure

TEST_F(NoahTest, MetaFailCreateTable) {
  auto try_create_table = [&](const std::string& tbl_name, int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      client_wrapper_.cluster_->CreateTable(tbl_name, 4, 3, 2);
      auto table_names = client_wrapper_.GetTableNames();
      return Contains(table_names, tbl_name);
    }, max_retry_num);
  };

  { // Leader fails before CreateTable
    int leader_index = meta_server_cluster_.StopLeader();
    ASSERT_NE(leader_index, -1);
    ASSERT_TRUE(try_create_table("table_1", 20));
    meta_server_cluster_.WaitForLeader();
    meta_server_cluster_.Start(leader_index);
  }

  { // Follower fails before CreateTable
    int follower_index = meta_server_cluster_.StopRandomFollower();
    ASSERT_NE(follower_index, -1);
    ASSERT_TRUE(try_create_table("table_2", 20));
  }
}

TEST_F(NoahTest, MetaFailDropTable) {
  auto try_drop_table = [&](const std::string& tbl_name, int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      client_wrapper_.cluster_->DropTable({tbl_name});
      auto table_names = client_wrapper_.GetTableNames();
      return !Contains(table_names, tbl_name);
    }, max_retry_num);
  };

  // Prepare: Create table
  client_wrapper_.cluster_->CreateTable("table_1", 4, 3, 100);
  client_wrapper_.cluster_->CreateTable("table_2", 4, 3, 100);
  client_wrapper_.cluster_->CreateTable("table_3", 4, 3, 100);
  client_wrapper_.cluster_->CreateTable("table_4", 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  std::sort(table_names.begin(), table_names.end());
  ASSERT_EQ(table_names,
            StringVector({"table_1", "table_2", "table_3", "table_4"}));

  { // Leader fails before DropTable
    int leader_index = meta_server_cluster_.StopLeader();
    ASSERT_NE(leader_index, -1);
    ASSERT_TRUE(try_drop_table("table_1", 20));
    meta_server_cluster_.WaitForLeader();
    meta_server_cluster_.Start(leader_index);
  }

  { // Follower fails before DropTable
    int follower_index = meta_server_cluster_.StopRandomFollower();
    ASSERT_NE(follower_index, -1);
    ASSERT_TRUE(try_drop_table("table_3", 20));
  }
}

TEST_F(NoahTest, MetaFailSet) {
  auto try_set = [&](const std::string& tbl_name, const KeyValueMap& key_values,
                     int32_t ttl, int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      client_wrapper_.cluster_->Set(tbl_name, key_values, ttl);
      auto keys = Keys(key_values);
      KeyValueMap key_values2{};
      client_wrapper_.cluster_->Get(tbl_name, keys, &key_values2);
      if (key_values2 != key_values) {
        return false;
      }

      if (ttl != -1) {
        sleep(static_cast<unsigned int>(ttl + 1));

        KeyValueMap dummy{};
        for (const auto& k : keys) {
          dummy[k] = "";
        }
        KeyValueMap key_values3{};
        client_wrapper_.cluster_->Get(tbl_name, keys, &key_values3);
        if (key_values3 != dummy) {
          return false;
        }
      }

      return true;
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  { // Leader fails before Set
    int leader_index = meta_server_cluster_.StopLeader();
    ASSERT_NE(leader_index, -1);
    ASSERT_TRUE(
      try_set(table_name, {{"Bush", "1924"}, {"Gorbachev", "1931"}}, -1, 20));
    ASSERT_TRUE(
      try_set(table_name, {{"Elizabeth", "1926"}, {"Elder", "1926"}}, 2, 20));
    meta_server_cluster_.WaitForLeader();
    meta_server_cluster_.Start(leader_index);
  }

  { // Follower fails before Set
    int follower_index = meta_server_cluster_.StopRandomFollower();
    ASSERT_NE(follower_index, -1);
    ASSERT_TRUE(
      try_set(table_name, {{"Bush", "94"}, {"Gorbachev", "87"}}, -1, 20));
    ASSERT_TRUE(
      try_set(table_name, {{"Elizabeth", "92"}, {"Elder", "92"}}, 2, 20));
  }
}

TEST_F(NoahTest, MetaFailGet) {
  auto try_get = [&](const std::string& tbl_name, const StringVector& keys,
                     const StringVector& expected_values, int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      KeyValueMap key_values{};
      client_wrapper_.cluster_->Get(tbl_name, keys, &key_values);
      for (std::size_t i = 0; i < keys.size(); i++) {
        if (key_values[keys[i]] != expected_values[i]) {
          return false;
        }
      }
      return true;
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Prepare: Set values to be retrieved
  client_wrapper_.cluster_->Set(table_name, {{"A", "1000"}, {"B", "2000"}}, -1);
  client_wrapper_.cluster_->Set(table_name, {{"C", "3000"}, {"D", "4000"}}, 1);
  usleep(1020 * 1000);

  { // Leader fails before Get
    int leader_index = meta_server_cluster_.StopLeader();
    ASSERT_NE(leader_index, -1);
    ASSERT_TRUE(
      try_get(table_name, {"A", "B", "C", "D"}, {"1000", "2000", "", ""}, 20));
    meta_server_cluster_.WaitForLeader();
    meta_server_cluster_.Start(leader_index);
  }

  // Prepare: Reset values to be retrieved
  client_wrapper_.cluster_->Set(table_name, {{"A", "1000"}, {"B", "2000"}}, -1);
  client_wrapper_.cluster_->Set(table_name, {{"C", "3000"}, {"D", "4000"}}, 1);
  usleep(1020 * 1000);

  { // Follower fails before Get
    int follower_index = meta_server_cluster_.StopRandomFollower();
    ASSERT_NE(follower_index, -1);
    ASSERT_TRUE(
      try_get(table_name, {"A", "B", "C", "D"}, {"1000", "2000", "", ""}, 20));
  }
}

TEST_F(NoahTest, MetaFailList) {
  auto try_list = [&](const std::string& tbl_name, int max_result_num,
                      const KeyValueMap& expected_key_values,
                      bool expected_is_end,
                      int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      IteratorUniPtr iterator{client_wrapper_.cluster_->NewIterator(tbl_name)};
      KeyValueMap key_values{};
      bool is_end = false;
      iterator->List(0, max_result_num, &key_values, &is_end);
      return key_values == expected_key_values && is_end == expected_is_end;
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Prepare: Set values to be listed
  client_wrapper_.cluster_->Set(
    table_name, {{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}},
    -1);

  { // Leader fails before List
    int leader_index = meta_server_cluster_.StopLeader();
    ASSERT_NE(leader_index, -1);
    ASSERT_TRUE(
      try_list(table_name, 5,
               {{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}},
               true, 20));
    meta_server_cluster_.WaitForLeader();
    meta_server_cluster_.Start(leader_index);
  }

  { // Follower fails before List
    int follower_index = meta_server_cluster_.StopRandomFollower();
    ASSERT_NE(follower_index, -1);
    ASSERT_TRUE(
      try_list(table_name, 5,
               {{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}},
               true, 20));
  }
}

TEST_F(NoahTest, MetaFailDel) {
  auto try_del = [&](const std::string& tbl_name, const StringVector& keys,
                     int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      client_wrapper_.cluster_->Delete(tbl_name, keys);
      KeyValueMap key_values{};
      client_wrapper_.cluster_->Get(tbl_name, keys, &key_values);
      for (const auto& k : keys) {
        if (!key_values[k].empty()) {
          return false;
        }
      }
      return true;
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Prepare: Set values to be deleted
  client_wrapper_.cluster_->Set(
    table_name, {{"A", "1000"}, {"B", "2000"}, {"C", "3000"}, {"D", "4000"}},
    -1);

  { // Leader fails before Del
    int leader_index = meta_server_cluster_.StopLeader();
    ASSERT_NE(leader_index, -1);
    ASSERT_TRUE(try_del(table_name, {"A", "B"}, 20));
    meta_server_cluster_.WaitForLeader();
    meta_server_cluster_.Start(leader_index);
  }

  { // Follower fails before Del
    int follower_index = meta_server_cluster_.StopRandomFollower();
    ASSERT_NE(follower_index, -1);
    ASSERT_TRUE(try_del(table_name, {"C", "D"}, 20));
  }
}

// ========= Test NodeServer failure

TEST_F(NoahTest, NodeFailCreateTable) {
  auto check_table_exist = [&](const std::string& tbl_name, int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      auto table_names = client_wrapper_.GetTableNames();
      return Contains(table_names, tbl_name);
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Stop node_server and check if table exists
  int node_index = node_server_cluster_.StopRandom();
  ASSERT_TRUE(check_table_exist(table_name, 20));

  // Start node_server and check if table exists
  node_server_cluster_.Start(node_index);
  ASSERT_TRUE(check_table_exist(table_name, 20));
}

TEST_F(NoahTest, NodeFailDropTable) {
  auto check_table_not_exist = [&](const std::string& tbl_name,
                                   int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      auto table_names = client_wrapper_.GetTableNames();
      return !Contains(table_names, tbl_name);
    }, max_retry_num);
  };

  // Prepare: Create table and then drop it
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  client_wrapper_.cluster_->DropTable({table_name});

  // Stop node_server and check if table NOT exists
  int node_index = node_server_cluster_.StopRandom();
  ASSERT_TRUE(check_table_not_exist(table_name, 20));

  // Start node_server and check if table NOT exists
  node_server_cluster_.Start(node_index);
  ASSERT_TRUE(check_table_not_exist(table_name, 20));
}

TEST_F(NoahTest, NodeFailSetAndGet) {
  auto check_key_values = [&](const std::string& tbl_name,
                              const KeyValueMap& expected_key_values,
                              int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      KeyValueMap kv{};
      client_wrapper_.cluster_->Get(tbl_name, Keys(expected_key_values), &kv);
      return kv == expected_key_values;
    }, max_retry_num);
  };

  auto check_set_get = [&](const std::string& tbl_name, int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      client_wrapper_.cluster_->Set(tbl_name, {{"X", "1"}}, -1);
      client_wrapper_.cluster_->Set(tbl_name, {{"Y", "2"}}, 1);
      usleep(1020 * 1000);

      KeyValueMap kv{};
      client_wrapper_.cluster_->Get(tbl_name, {"X", "Y"}, &kv);
      return kv == KeyValueMap{{"X", "1"}, {"Y", ""}};
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Prepare: Set values
  KeyValueMap kv1{{"A", "2"}, {"B", "4"}};
  KeyValueMap kv2{{"C", "6"}, {"D", "8"}};
  KeyValueMap kv_expected{{"A", "2"}, {"B", "4"}, {"C", ""}, {"D", ""}};
  client_wrapper_.cluster_->Set(table_name, kv1, -1);
  client_wrapper_.cluster_->Set(table_name, kv2, 1);

  {
    // Stop node_server and check if values persist
    int node_index = node_server_cluster_.StopRandom();
    usleep(1020 * 1000);
    ASSERT_TRUE(check_key_values(table_name, kv_expected, 20));

    // Start node_server and check if values persist
    node_server_cluster_.Start(node_index);
    ASSERT_TRUE(check_key_values(table_name, kv_expected, 20));
  }

  {
    // Stop node_server and check if Set and Get work correctly
    int node_index = node_server_cluster_.StopRandom();
    ASSERT_TRUE(check_set_get(table_name, 20));

    // Start node_server and check if Set and Get work correctly
    node_server_cluster_.Start(node_index);
    ASSERT_TRUE(check_set_get(table_name, 20));
  }
}

TEST_F(NoahTest, NodeFailList) {
  auto check_list = [&](const std::string& tbl_name, int max_result_num,
                        const KeyValueMap& expected_key_values,
                        bool expected_is_end,
                        int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      IteratorUniPtr iterator{client_wrapper_.cluster_->NewIterator(tbl_name)};
      KeyValueMap key_values{};
      bool is_end = false;
      iterator->List(0, max_result_num, &key_values, &is_end);
      return key_values == expected_key_values && is_end == expected_is_end;
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Prepare: Set values to be listed
  KeyValueMap key_values = {{"A", "1000"}, {"B", "2000"}, {"C", "3000"},
                            {"D", "4000"}};
  client_wrapper_.cluster_->Set(table_name, key_values, -1);

  // Stop node_server and check if List works
  int node_index = node_server_cluster_.StopRandom();
  ASSERT_TRUE(check_list(table_name, 5, key_values, true, 20));

  // Start node_server and check if List works
  node_server_cluster_.Start(node_index);
  ASSERT_TRUE(check_list(table_name, 5, key_values, true, 20));
}

TEST_F(NoahTest, NodeFailDel) {
  auto check_key_values = [&](const std::string& tbl_name,
                              const KeyValueMap& expected_key_values,
                              int max_retry_num) {
    return RetryCheck([&] {
      meta_server_cluster_.WaitForLeader();

      KeyValueMap kv{};
      client_wrapper_.cluster_->Get(tbl_name, Keys(expected_key_values), &kv);
      return kv == expected_key_values;
    }, max_retry_num);
  };

  // Prepare: Create table
  auto table_name = "table_1";
  client_wrapper_.cluster_->CreateTable(table_name, 4, 3, 100);
  auto table_names = client_wrapper_.GetTableNames();
  ASSERT_EQ(table_names, StringVector({table_name}));

  // Prepare: Set values and then delete some of them
  client_wrapper_.cluster_->Set(
    table_name, {{"A", "1"}, {"B", "2"}, {"C", "3"}}, -1);
  client_wrapper_.cluster_->Delete(table_name, {"A", "B"});
  KeyValueMap expected_kvs = {{"A", ""}, {"B", ""}, {"C", "3"}};

  // Stop node_server and check if values deleted
  int node_index = node_server_cluster_.StopRandom();
  ASSERT_TRUE(check_key_values(table_name, expected_kvs, 20));

  // Start node_server and check if values deleted
  node_server_cluster_.Start(node_index);
  ASSERT_TRUE(check_key_values(table_name, expected_kvs, 20));
}

}  // namespace client
}  // namespace noah
