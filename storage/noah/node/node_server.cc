// Author: Zhenwei Zhu losted.leo@gmail.com

#include "brpc/controller.h"
#include "butil/endpoint.h"
#include "butil/hash.h"
#include "braft/configuration.h"
#include "butil/time/time.h"
#include "butil/time.h"

#include "meta/meta.pb.h"
#include "node/node_server.h"
#include "node/node.pb.h"
#include "node/node_migrate.h"
#include "node/cache.h"

#include "leveldb/db.h"

DEFINE_int32(node_max_retry, 1, "Node request retry number");
DEFINE_int32(expired_start_hour, 0, "CheckExpired start in hour");
DEFINE_int32(expired_end_hour, 6, "CheckExpired end in hour");
DEFINE_int32(bg_thread_num, 2, "NodeServer background thread number");
DEFINE_bool(open_persistence, false, "whether oepn persistence");
DECLARE_bool(open_statistic);

const int32_t kMinDuplicateNumber = 1;
const int32_t kMaxDuplicateNumber = 5;

namespace noah {
namespace node {
ServerTimer::ServerTimer() : node_server_(NULL) {
}

ServerTimer::~ServerTimer() {
}

int ServerTimer::Init(NodeServer* node_server, int timeout_ms) {
  if (0 != RepeatedTimerTask::init(timeout_ms)) {
    LOG(ERROR) << "ServerTimer init error";
    return -1;
  }
  node_server_ = node_server;
  return 0;
}

void ServerTimer::on_destroy() {
  node_server_ = NULL;
}

void PingTimer::run() {
  if (node_server_) {
    node_server_->HandlePing();
  }
}

void StatisticsTimer::run() {
  if (node_server_) {
    node_server_->HandleStatistics();
  }
}

void ExpiredTimer::run() {
  if (node_server_) {
    node_server_->CheckExpired();
  }
}

MetaUpdater::MetaUpdater(NodeServer* node_server, const VersionInfo& info)
  : node_server_(node_server), info_(info) {
}

void MetaUpdater::Run() {
  node_server_->UpdateMetaInfo(info_);
  delete this;
}

DropTabler::DropTabler(NodeServer* node_server, TableMap* tables)
  : node_server_(node_server) {
    tables_.swap(*tables);
}

void DropTabler::Run() {
  TableArray tables;
  for (auto& table : tables_) {
    tables.push_back(std::move(table.second));
  }
  node_server_->DropTable(tables);
  delete this;
}

NodeServer::NodeServer(const NodeServerOptions& options)
  : options_(options),
    meta_client_(options_.meta_group, options_.meta_conf),
    update_pool_("meta_update", 1),
    migrate_pool_("node_migrate", 1),
    bg_pool_("back_ground", FLAGS_bg_thread_num),
    node_data_(this) {
  self_node_.ip = options_.ip;
  self_node_.port = options_.port;
  noah::common::ChannelOptions channel_options;
  channel_options.timeout_ms = options_.node_timeout_ms;
  channel_options.max_retry = FLAGS_node_max_retry;
  channel_options.protocol = "baidu_std";
  channels_.reset(new Channels(channel_options));
  server_state_ = ServerState::kStart;
}

bool NodeServer::LoadDataFormDb() {
  GoogleMap key_values;
  std::vector<int32_t> ttl_vec;
  char type = 1;
  uint32_t batch_size = 1000000;

  for (int i = 0; i < db_num_; i++) {
    leveldb::Iterator* it = db_[i]->NewIterator(read_options_);
    key_values.clear();
    ttl_vec.clear();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string key_str = it->key().ToString();
      std::string value_str = it->value().ToString();
      std::string table_name, ori_key;

      int part_id;
      if (!noah::common::DecodeLbKey(key_str, &table_name, &part_id, &type, &ori_key)) {
        LOG(INFO) << "decode failled! " << key_str;
        continue;
      } else {
        LOG(INFO) << "decod key  " << ori_key << " get_key " << key_str <<  " table "
          << table_name << " id " << part_id <<  " value "<< value_str;
      }

      ValueInfo value_info;
      int32_t current_time = butil::gettimeofday_s();
      value_info.ParseFromString(value_str);
      if (value_info.expired() && value_info.expired() <= current_time) {
        continue;
      }

      key_values[ori_key] = value_info.value();
      ttl_vec.push_back(value_info.expired()? value_info.expired():-1);
      if (key_values.size() > batch_size) {
        node_data_.Set(table_name, part_id, key_values, ttl_vec);
        key_values.clear();
        ttl_vec.clear();
      }
    }
    if (key_values.size() > 0) {
      key_values.clear();
      ttl_vec.clear();
    }
  }

  return true;
}

void NodeServer::CheckLdbExpired() {
  std::vector<std::string> keys;
  uint32_t batch_size = 100000;
  for (int i = 0; i < db_num_; i++) {
    const leveldb::Snapshot* snapshot = db_[i]->GetSnapshot();
    leveldb::ReadOptions read_options;
    leveldb::WriteOptions write_options;
    read_options.snapshot = snapshot;
    leveldb::Iterator* it = db_[i]->NewIterator(read_options);
    keys.clear();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string key_str = it->key().ToString();
      std::string value_str = it->value().ToString();

      ValueInfo value_info;
      int32_t current_time = butil::gettimeofday_s();
      value_info.ParseFromString(value_str);
      if (value_info.expired() && value_info.expired() <= current_time) {
        keys.push_back(key_str);
        if (keys.size() >= batch_size) {
          break;
        }
      }
    }
    delete it;
    leveldb::WriteBatch batch;
    for (const auto& key : keys) {
      batch.Delete(key);
    }

    leveldb::Status s = db_[i]->Write(write_options, &batch);
    if (!s.ok()) {
      db_[i]->ReleaseSnapshot(read_options.snapshot);
      LOG(INFO) << "delete db " << i << "falied!";
      continue;
    }
    keys.clear();
    db_[i]->ReleaseSnapshot(read_options.snapshot);
  }
}

NodeServer::~NodeServer() {
}

int NodeServer::Start() {
  if (FLAGS_open_persistence) {
    if (!node_data_.LoadDataFormDb()) {
      LOG(INFO) << "load data from DB fialed! check db files exsit!";
    }
  }
  if (!meta_client_.Init()) {
    return -1;
  }
  if (0 != ping_timer_.Init(this, options_.ping_interval_ms)) {
    return -1;
  }
  if (0 != expired_time_.Init(this, options_.expired_interval_ms)) {
    return -1;
  }
  if (0 != statistics_timer_.Init(this, options_.statistics_interval_ms)) {
    return -1;
  }

  server_state_ =  ServerState::kRun;
  ping_timer_.start();
  statistics_timer_.start();
  expired_time_.start();
  update_pool_.Start();
  migrate_pool_.Start();
  bg_pool_.Start();

  ping_timer_.run_once_now();
  return 0;
}

void NodeServer::ShutDown() {
  ping_timer_.destroy();
  expired_time_.destroy();
  update_pool_.JoinAll();
  migrate_pool_.JoinAll();
  bg_pool_.JoinAll();
}

void NodeServer::CreateTable(const CreateTableRequest* request,
                             CreateTableResponse* response) {
  Partition snap_partition;
  if (!CheckEpoch(request->epoch(), request->version(), request->table_name(),
                  0, &snap_partition)) {
    response->set_status(Status::EEPOCH);
    return;
  }
  if (0 == snap_partition.id()) {
    response->set_status(Status::EXIST);
    return;
  }

  TableInfo table_info(request->table_name());
  table_info.partition_num = request->parition_num();
  table_info.duplicate_num = request->duplicate_num();
  table_info.capacity = request->capacity();
  if (table_info.duplicate_num < kMinDuplicateNumber) {
    table_info.duplicate_num = kMinDuplicateNumber;
  }
  if (table_info.duplicate_num > kMaxDuplicateNumber) {
    table_info.duplicate_num = kMaxDuplicateNumber;
  }
  if (table_info.duplicate_num > (int32_t)meta_info_.nodes.size()) {
    table_info.duplicate_num = meta_info_.nodes.size();
  }
  VersionInfo info = CreateTable(meta_info_.version, table_info);
  if (info.epoch < 0) {
    response->set_status(Status::ERROR);
    return;
  }
  response->set_status(Status::OK);
  response->set_epoch(info.epoch);
  response->set_version(info.version);
  return;
}

void NodeServer::Get(const GetRequest* request, GetResponse* response) {
  Partition snap_partition;
  if (!CheckEpoch(request->epoch(), request->version(), request->table_name(),
                  request->id(), &snap_partition)) {
    response->set_status(Status::EEPOCH);
    return;
  }
  if (-1 == snap_partition.id()) {
    response->set_status(Status::NOTFOUND);
    return;
  }

  VLOG(10) << "Table " << request->table_name() << " id " << request->id()
           << " partition" << snap_partition.DebugDump();
  Node master = snap_partition.master();
  if (self_node_ != master) {
    response->set_status(Status::NOTMASTER);
    return;
  }

  if (node_data_.Get(request->table_name(), request->id(), request->keys(),
                     response->mutable_values())) {
    response->set_status(Status::OK);
  } else {
    response->set_status(Status::ERROR);
  }

  if (FLAGS_open_persistence) {
    if (node_data_.LdbGet(request->table_name(), request->id(), request->keys(),
        response->mutable_values())) {
      response->set_status(Status::OK);
    } else {
      response->set_status(Status::ERROR);
    }
  }
  return;
}

bool NodeServer::LdbSet(const std::string table_name, int32_t id, const GoogleMap& key_values,
                        int32_t expired) {
  ValueInfo value_info;
  leveldb::WriteBatch batch;
  for (const auto& value : key_values) {
    value_info.set_value(value.second);
    value_info.set_expired(expired);
    std::string encode_key;
    if (!noah::common::EncodeLbKey(value.first, table_name, id,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      LOG(INFO) << "Encode failed! " << table_name << " key" << value.first;
      continue;
    }
    batch.Put(encode_key, value_info.SerializeAsString());
  }

  uint32_t db_index = butil::Hash(table_name + std::to_string(id)) % db_num_;
  leveldb::Status s = db_[db_index]->Write(write_options_, &batch);
  if (!s.ok()) {
    LOG(INFO) << "Ldb set failed  " << table_name;
    return false;
  }

  return true;
}

bool NodeServer::LdbGet(const std::string& table_name, int32_t id, const GoogleRepeated& keys,
                        GoogleRepeated* values) {
  ValueInfo value_info;
  uint32_t db_index = butil::Hash(table_name + std::to_string(id)) % db_num_;
  for (const auto& key : keys) {
    std::string value;
    std::string encode_key;
    if (!noah::common::EncodeLbKey(key, table_name, id,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      LOG(INFO) << "LDB get encode failed " << table_name << " key " << key;
      *(values->Add()) = std::string();
      continue;
    }

    leveldb::Status s = db_[db_index]->Get(read_options_, encode_key, &value);
    if (s.ok()) {
      LOG(INFO) << "Ldb get success  " << table_name  << " " <<  id << " key " << key
        << " v " << value;
      value_info.ParseFromString(value);
      *(values->Add()) = value_info.value();
    } else {
      LOG(INFO) << "Ldb get failed" << table_name  << " " <<  id << " key " << key
        << " v " << value;
      *(values->Add()) = std::string();
      return false;
    }
  }
  return true;
}

bool NodeServer::LdbDel(const std::string table_name, int32_t id, const GoogleRepeated& keys) {
  leveldb::WriteBatch batch;
  for (const auto& key : keys) {
    std::string value;
    std::string encode_key;
    if (!noah::common::EncodeLbKey(key, table_name, id,
          static_cast<char>(noah::common::KeyType::kKey), &encode_key)) {
      LOG(INFO) << "LDB get encode failed " << table_name << " key " << key;
      continue;
    }
    batch.Delete(encode_key);
  }

  uint32_t db_index = butil::Hash(table_name + std::to_string(id)) % db_num_;
  leveldb::Status s = db_[db_index]->Write(write_options_, &batch);
  if (!s.ok()) {
    LOG(INFO) << "Ldb set failed  " << table_name;
    return false;
  }

  return true;
}

void NodeServer::Set(const SetRequest* request, SetResponse* response) {
  Partition snap_partition;
  if (!CheckEpoch(request->epoch(), request->version(), request->table_name(),
                  request->id(), &snap_partition)) {
    response->set_status(Status::EEPOCH);
    return;
  }
  if (-1 == snap_partition.id()) {
    response->set_status(Status::NOTFOUND);
    return;
  }
  if (snap_partition.state() >= noah::common::Partition::State::kSlowDown) {
    LOG(ERROR) << "Table: " << request->table_name() << " id: " << request->id()
               << " not active";
    response->set_status(Status::NOTACTIVE);
    return;
  }

  VLOG(10) << "Table " << request->table_name() << " id " << request->id()
           << " partition" << snap_partition.DebugDump();
  Node master = snap_partition.master();
  if (self_node_ == master) {
    std::vector<Node> slaves = snap_partition.slaves();
    NodeSet nodes;
    if (NodeMigrate::CheckMigrating(request->table_name(), request->id(), &nodes)) {
      slaves.insert(slaves.end(), nodes.begin(), nodes.end());
    }
    if (slaves.size()) {
      ParallelChannelPtr pchan = channels_->GetParallelChannelByNodes(slaves);
      NodeService_Stub stub(reinterpret_cast<google::protobuf::RpcChannel*>(pchan.get()));
      SetResponse slave_response;
      brpc::Controller cntl;
      stub.Set(&cntl, request, &slave_response, NULL);
      if (cntl.Failed()) {
        response->set_status(Status::ERROR);
        LOG(ERROR) << "Failed to request slaves table: " << request->table_name()
                   << " id: " << request->id() << " error: " << cntl.ErrorText();
        return;
      }
      if (Status::OK != slave_response.status()) {
        LOG(ERROR) << "Set to slaves table: " << request->table_name() << " id: "
                   << request->id() << " error: " << slave_response.DebugString();
        response->set_status(Status::ERROR);
        return;
      }
    } else {
      VLOG(10) << "Table " << request->table_name() << " partition id " << request->id()
                   << " not exist slaves";
    }
  }

  if (node_data_.Set(request->table_name(), request->id(), request->key_values(),
                     request->expired())) {
    response->set_status(Status::OK);
  } else {
    response->set_status(Status::ERROR);
  }

  if (FLAGS_open_persistence) {
    if (node_data_.LdbSet(request->table_name(), request->id(), request->key_values(),
        request->expired())) {
      response->set_status(Status::OK);
    } else {
      response->set_status(Status::ERROR);
    }
  }

  return;
}

void NodeServer::Del(const DelRequest* request, DelResponse* response) {
  Partition snap_partition;
  if (!CheckEpoch(request->epoch(), request->version(), request->table_name(),
                  request->id(), &snap_partition)) {
    response->set_status(Status::EEPOCH);
    return;
  }
  if (-1 == snap_partition.id()) {
    response->set_status(Status::NOTFOUND);
    return;
  }
  if (snap_partition.state() > noah::common::Partition::State::kSlowDown) {
    LOG(ERROR) << "Table: " << request->table_name() << " id: " << request->id()
               << " not active";
    response->set_status(Status::NOTACTIVE);
    return;
  }

  VLOG(10) << "Table " << request->table_name() << " id " << request->id()
           << " partition" << snap_partition.DebugDump();
  Node master = snap_partition.master();
  if (self_node_ == master) {
    std::vector<Node> slaves = snap_partition.slaves();
    NodeSet nodes;
    if (NodeMigrate::CheckMigrating(request->table_name(), request->id(), &nodes)) {
      slaves.insert(slaves.end(), nodes.begin(), nodes.end());
    }
    if (slaves.size()) {
      ParallelChannelPtr pchan = channels_->GetParallelChannelByNodes(slaves);
      NodeService_Stub stub(reinterpret_cast<google::protobuf::RpcChannel*>(pchan.get()));
      DelResponse slave_response;
      brpc::Controller cntl;
      stub.Del(&cntl, request, &slave_response, NULL);
      if (cntl.Failed()) {
        response->set_status(Status::ERROR);
        LOG(ERROR) << "Failed to request slaves table: " << request->table_name()
                   << " id: " << request->id() << " error: " << cntl.ErrorText();
        return;
      }
      if (Status::OK != slave_response.status()) {
        LOG(ERROR) << "Del to slaves table: " << request->table_name() << " id: "
                   << request->id() << " error: " << slave_response.DebugString();
        response->set_status(Status::ERROR);
        return;
      }
    } else {
      VLOG(30) << "Table " << request->table_name() << " partition id " << request->id()
                   << " not exist slaves";
    }
  }

  if (node_data_.Del(request->table_name(), request->id(), request->keys())) {
    if (FLAGS_open_persistence) {
      if (node_data_.LdbDel(request->table_name(), request->id(), request->keys())) {
        response->set_status(Status::OK);
        return;
      }
    } else {
      response->set_status(Status::OK);
      return;
    }
  }

  response->set_status(Status::ERROR);
  return;
}

void NodeServer::List(const ListRequest* request, ListResponse* response) {
  Partition snap_partition;
  if (!CheckEpoch(request->epoch(), request->version(), request->table_name(),
                  request->id(), &snap_partition)) {
    response->set_status(Status::EEPOCH);
    return;
  }
  if (-1 == snap_partition.id()) {
    response->set_status(Status::NOTFOUND);
    return;
  }

  VLOG(10) << "Table " << request->table_name() << " id " << request->id()
           << " partition" << snap_partition.DebugDump();
  Node master = snap_partition.master();
  if (self_node_ != master) {
    response->set_status(Status::NOTMASTER);
    return;
  }
  int32_t total_size = 0;
  bool end = true;
  std::string last_key;
  ListInfo info(request->offset(), request->size(), request->offset_key());
  if (node_data_.List(request->table_name(), request->id(), info,
                      response->mutable_key_values(), &total_size, &end,
                      &last_key)) {
    response->set_status(Status::OK);
    response->set_total_size(total_size);
    response->set_end(end);
    if (!end) {
      response->set_key(last_key);
    }
  } else {
    response->set_status(Status::ERROR);
  }
  return;
}

void NodeServer::Migrate(const MigrateRequest* request, MigrateResponse* response) {
  if (!CheckEpoch(request->epoch(), request->version(), request->table_name(),
                  request->id(), NULL)) {
    response->set_status(Status::EEPOCH);
    return;
  }

  if (node_data_.Migrate(request->table_name(), request->id(), request->key_values(),
                         request->expired())) {
    response->set_status(Status::OK);
  } else {
    response->set_status(Status::ERROR);
  }
  return;
}

void NodeServer::HandleStatistics() {
  if (!FLAGS_open_statistic) {
    return;
  }
  StatisticsTableSize();
}

void NodeServer::HandlePing() {
  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::Type::PING);
  request.mutable_version()->set_epoch(meta_info_.version.epoch);
  request.mutable_version()->set_version(meta_info_.version.version);
  request.mutable_ping()->mutable_node()->set_ip(options_.ip);
  request.mutable_ping()->mutable_node()->set_port(options_.port);

  if (!meta_client_.MetaCmd(&request, &response, options_.ping_interval_ms / 2)) {
    return;
  }

  if (noah::meta::Status::OK == response.status()) {
    if (CheckShouldUpdate(VersionInfo(response.version()))) {
      MetaUpdater* updater = new MetaUpdater(this, VersionInfo(response.version()));
      update_pool_.AddWork(updater);
    }
  } else if (noah::meta::Status::REDIRECT == response.status()) {
    LOG(INFO) << "Meta leader change to " << response.redirect();
    meta_client_.UpdateLeader(response.redirect());
  } else {
    LOG(ERROR) << "Ping to meta server error " << response.DebugString();
  }
}

void NodeServer::CheckExpired() {
  butil::Time time = butil::Time::Now();
  butil::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  if (exploded.hour >= FLAGS_expired_start_hour &&
      exploded.hour <= FLAGS_expired_end_hour) {
    node_data_.CheckExpired();
    if (FLAGS_open_persistence) {
      node_data_.CheckLdbExpired();
    }
  }
  VLOG(10) << exploded.year << " " << exploded.month << " " << exploded.day_of_month
           << " " << exploded.hour << " " << exploded.minute << " " << exploded.second;
}

bool NodeServer::UpdateMetaInfo(const VersionInfo& info) {
  if (!CheckShouldUpdate(info)) {
    return true;
  }

  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::Type::PULL);
  request.mutable_version()->set_epoch(info.epoch);
  request.mutable_version()->set_version(info.version);
  request.mutable_pull()->mutable_node()->set_ip(options_.ip);
  request.mutable_pull()->mutable_node()->set_port(options_.port);

  if (!meta_client_.MetaCmd(&request, &response, options_.node_timeout_ms)) {
    return false;
  }

  if (noah::meta::Status::OK == response.status()) {
    butil::WriterAutoLock lock_w(&meta_lock_);
    meta_info_.version = VersionInfo(response.version());
    TableMap delete_tables;
    delete_tables.swap(meta_info_.table_info);
    for (const auto& table : response.pull().info()) {
      meta_info_.table_info[table.name()] = noah::common::Table(table);
      delete_tables.erase(table.name());
    }

    meta_info_.nodes.clear();
    for (const auto& value : response.pull().node_status()) {
      meta_info_.nodes.push_back(noah::common::NodeStatus(value));
    }

    if (delete_tables.size()) {
      DropTabler* drop_tabler = new DropTabler(this, &delete_tables);
      bg_pool_.AddWork(drop_tabler);
    }
  } else if (noah::meta::Status::REDIRECT == response.status()) {
    LOG(INFO) << "Meta leader change to " << response.redirect();
    meta_client_.UpdateLeader(response.redirect());
    return UpdateMetaInfo(VersionInfo(response.version()));
  } else {
    LOG(ERROR) << "Update meta info error " << response.DebugString();
    return false;
  }

  return true;
}

void NodeServer::DropTable(const TableArray& tables) {
  CacheMap caches;
  node_data_.ClearTables(tables, &caches);
  butil::Timer timer;
  auto iter = caches.begin();
  std::string cache_str;
  for (; iter != caches.end(); ) {
    cache_str = iter->first;
    timer.start();

    // first delete cahce mem then delete iter
    if (iter->second != nullptr) {
      iter->second->Clear();
    }
    iter = caches.erase(iter);
    timer.stop();
    LOG(INFO) << "Clear cache " << cache_str << " cost " << timer.u_elapsed() << " us";
  }

  if (FLAGS_open_persistence) {
    node_data_.ClearLdbTables(tables);
  }
}

bool NodeServer::FindTable(const std::string& table_name, Table* table) {
  butil::ReaderAutoLock lock_r(&meta_lock_);
  auto iter = meta_info_.table_info.find(table_name);
  if (iter == meta_info_.table_info.end()) {
    return false;
  }
  *table = iter->second;
  return true;
}

bool NodeServer::CheckEpoch(int64_t epoch, int64_t version, const std::string& table_name,
                            int32_t id, Partition* snap_info, bool updated /* false */) {
  if (!updated) {
    bool should_update = false;
    {
      butil::ReaderAutoLock lock_r(&meta_lock_);
      if (epoch > meta_info_.version.epoch || version > meta_info_.version.version) {
        should_update = true;
      }
    }
    if (should_update) {
      if (UpdateMetaInfo(VersionInfo(epoch, version))) {
        return CheckEpoch(epoch, version, table_name, id, snap_info, true);
      } else {
        return false;
      }
    }
  }
  {
    butil::ReaderAutoLock lock_r(&meta_lock_);
    if (meta_info_.version.epoch == epoch) {
      if (!snap_info) {
        return true;
      }
      auto iter = meta_info_.table_info.find(table_name);
      if (iter == meta_info_.table_info.end()) {
        LOG(INFO) << "Table " << table_name <<" not exist";
        return true;
      }
      if (version != iter->second.version()) {
        LOG(ERROR) << "Table " << table_name << " current version: "
                   << iter->second.version() << " request version: "
                   << version;
        return false;
      }
      const Partition* partition = iter->second.GetPartitionById(id);
      if (!partition) {
        LOG(ERROR) << " Table " << table_name << " partition not found " << table_name
                   << " id: " << id;
        return true;
      }
      *snap_info = *partition;
      return true;
    }
    LOG(ERROR) << "Epoch error request epoch " << epoch << " meta epoch "
               << meta_info_.version.epoch;
    return false;
  }

  return true;
}

VersionInfo NodeServer::CreateTable(const VersionInfo& info, const TableInfo& table_info) {
  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::INIT);
  request.mutable_version()->set_epoch(info.epoch);
  request.mutable_version()->set_version(info.version);
  uint32_t table_hash = butil::Hash(table_info.table_name);

  noah::common::NodeArray nodes;
  {
    butil::ReaderAutoLock lock_r(&meta_lock_);
    nodes = meta_info_.nodes;
  }
  if (nodes.empty()) {
    return VersionInfo();
  }

  noah::meta::Table* table = request.mutable_init()->mutable_table();
  table->set_name(table_info.table_name);
  table->set_capacity(table_info.capacity);
  table->set_duplicate_num(table_info.duplicate_num);
  int32_t start_index = table_hash % nodes.size();
  for (int i = 0; i < table_info.partition_num; ++i) {
    noah::meta::Partition* partition = table->add_partitions();
    partition->set_id(i);
    for (int j = 0; j < table_info.duplicate_num; ++j) {
      int node_id = (start_index + i + j) % nodes.size();
      if (0 == j) {
        partition->mutable_master()->set_ip(nodes[node_id].node().ip);
        partition->mutable_master()->set_port(nodes[node_id].node().port);
      } else {
        auto slave = partition->add_slaves();
        slave->set_ip(nodes[node_id].node().ip);
        slave->set_port(nodes[node_id].node().port);
      }
    }
  }
  VLOG(10) << "Create table request " << request.DebugString();
  if (!meta_client_.MetaCmd(&request, &response, options_.node_timeout_ms)) {
    return VersionInfo();
  }
  if (noah::meta::Status::OK == response.status()) {
    butil::WriterAutoLock lock_w(&meta_lock_);
    meta_info_.version = VersionInfo(response.version());
    table->set_version(response.version().version());
    meta_info_.table_info[table_info.table_name] = noah::common::Table(*table);
    return VersionInfo(response.version());
  }
  return VersionInfo();
}

bool NodeServer::CheckShouldUpdate(const VersionInfo& info) {
  butil::ReaderAutoLock lock_r(&meta_lock_);
  if (info.epoch != meta_info_.version.epoch
      || info.version != meta_info_.version.version) {
    return true;
  }
  return false;
}

void NodeServer::StatisticsTableSize() {
  if (!FLAGS_open_statistic) {
    return;
  }
}

}  // namespace node
}  // namespace noah
