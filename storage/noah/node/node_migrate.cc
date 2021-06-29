// Author: Zhenwei Zhu losted.leo@gmail.com

#include "node/node_migrate.h"

#include "butil/synchronization/rw_mutex.h"
#include "brpc/controller.h"
#include "butil/time.h"

#include "meta/meta.pb.h"
#include "node/node_server.h"
#include "node/cache.h"
#include "node/node.pb.h"
#include "node/node_data.h"

DEFINE_int32(migrate_batch_size, 10000, "Batch size to migrate partition");

namespace noah {
namespace node {
MigrateTaskInfo::MigrateTaskInfo(const noah::meta::MigrateUnit& unit)
  : table_name(unit.name()), partition_id(unit.id()),
    fault(unit.fault()), src(unit.src()), dst(unit.dst()) {
  if (noah::meta::Role::MASTER == unit.role()) {
    role = Role::kMaster;
  } else {
    role = Role::kSlave;
  }
}

void MigrateTaskInfo::Convert2MigrateUnit(noah::meta::MigrateUnit* unit) {
  unit->set_name(table_name);
  unit->set_id(partition_id);
  unit->mutable_fault()->set_ip(fault.ip);
  unit->mutable_fault()->set_port(fault.port);
  unit->mutable_src()->set_ip(src.ip);
  unit->mutable_src()->set_port(src.port);
  unit->mutable_dst()->set_ip(dst.ip);
  unit->mutable_dst()->set_port(dst.port);
  if (Role::kMaster == role) {
    unit->set_role(noah::meta::Role::MASTER);
  } else {
    unit->set_role(noah::meta::Role::SLAVE);
  }
}

NodeMigrate::NodeMigrate(const MigrateTaskInfo& task, NodeServer* node_server)
  : canceled_(false), task_info_(task), node_server_(node_server) {
  cache_ = node_server_->node_data_.GetCache(task_info_.table_name,
                                             task_info_.partition_id);
}

butil::RWMutex NodeMigrate::info_lock_;
StringNodeMap NodeMigrate::migrating_info_;

void NodeMigrate::Run() {
  if (!cache_.get()) {
    LOG(ERROR) << "Cache should exist " << task_info_.table_name << " id "
               << task_info_.partition_id;
    delete this;
    return;
  }

  butil::Timer timer;
  timer.start();

  // 1. Register info
  RegisterMigrateInfo();
  // 2. Migrate partition data
  bool result = MigrateData();
  // 3. Commit to meta_server
  if (result) {
    CommitToMeta();
  }
  // 4. Unregister info
  UnregisterMigrateInfo();
  timer.stop();
  LOG(INFO) << "Migrate " << task_info_.table_name << " id " << task_info_.partition_id
            << " to dst " << task_info_.dst.ToString() << " completed size "
            << cache_->key_values_.size() << " result " << (int32_t)result
            << " cost " << timer.u_elapsed() << " us";

  delete this;
}

void NodeMigrate::Cancel() {
  canceled_ = true;
}

bool NodeMigrate::CheckMigrating(const std::string& table_name, int32_t id,
                                 NodeSet* nodes) {
  if (migrating_info_.empty()) {
    return false;
  }
  std::string table_str = TableInfoString(table_name, id);
  butil::ReaderAutoLock lock_r(&info_lock_);
  auto iter = migrating_info_.find(table_str);
  if (iter != migrating_info_.end()) {
    *nodes = iter->second;
    return true;
  }
  return false;
}

bool NodeMigrate::MigrateData() {
  MigrateRequest request;
  MigrateResponse response;
  request.set_table_name(task_info_.table_name);
  request.set_id(task_info_.partition_id);
  request.set_expired(cache_->expired_);
  {
    butil::ReaderAutoLock lock_r(&node_server_->meta_lock_);
    request.set_epoch(node_server_->meta_info_.version.epoch);
    auto iter = node_server_->meta_info_.table_info.find(task_info_.table_name);
    if (iter == node_server_->meta_info_.table_info.end()) {
      LOG(ERROR) << "Table " << task_info_.table_name << " should exist";
      return false;
    }
    request.set_version(iter->second.version());
  }

  std::string last_key;
  Map::iterator iter;
  Map& key_values = cache_->key_values_;
  size_t buckets = key_values.bucket_count();
  bool end = false;
  while (!end && !canceled_ && !brpc::IsAskedToQuit()) {
    int32_t batch_size = 0;
    {
      butil::ReaderAutoLock lock_r(&cache_->lock_);
      if (!last_key.empty() && buckets == key_values.bucket_count()) {
        iter = key_values.find(last_key);
        if (iter == key_values.end()) {
          iter = key_values.begin();
        }
      } else {
        iter = key_values.begin();
        buckets = key_values.bucket_count();
      }
      while (iter != key_values.end()) {
        (*request.mutable_key_values())[iter->first] = iter->second;
        ++batch_size;
        ++iter;
        if (batch_size < FLAGS_migrate_batch_size) {
          continue;
        } else {
          break;
        }
      }
      if (iter != key_values.end()) {
        last_key = iter->first;
      } else {
        end = true;
      }
    }
    if (batch_size) {
      MigrateToDst(&request, &response);
    }
    request.clear_key_values();
  }

  return true;
}

void NodeMigrate::RegisterMigrateInfo() {
  std::string table_str = TableInfoString(task_info_.table_name,
                                          task_info_.partition_id);
  butil::WriterAutoLock lock_w(&info_lock_);
  auto iter = migrating_info_.find(table_str);
  if (iter == migrating_info_.end()) {
    NodeSet nodes;
    nodes.insert(task_info_.dst);
    migrating_info_[table_str] = nodes;
  } else {
    iter->second.insert(task_info_.dst);
  }
}

void NodeMigrate::UnregisterMigrateInfo() {
  std::string table_str = TableInfoString(task_info_.table_name,
                                          task_info_.partition_id);
  butil::WriterAutoLock lock_w(&info_lock_);
  auto iter = migrating_info_.find(table_str);
  if (iter != migrating_info_.end()) {
    iter->second.erase(task_info_.dst);
    if (iter->second.empty()) {
      migrating_info_.erase(iter);
    }
  }
}

bool NodeMigrate::MigrateToDst(MigrateRequest* request, MigrateResponse* response) {
  bool success = false;
  brpc::Controller cntl;
  ChannelPtr channel = node_server_->channels_->GetChannelByNode(task_info_.dst);
  NodeService_Stub stub(reinterpret_cast<google::protobuf::RpcChannel*>(channel.get()));
  do {
    stub.Migrate(&cntl, request, response, NULL);
    if (cntl.Failed()) {
      LOG(ERROR) << "Failed to send dst " << task_info_.dst.ToString()
                 << " error " << cntl.ErrorText();
      cntl.Reset();
      usleep(500 * 1000L);
      continue;
    }
    cntl.Reset();
    if (Status::OK == response->status()) {
      success = true;
      break;
    } else if (Status::EEPOCH == response->status()) {
      if (node_server_->UpdateMetaInfo(VersionInfo(request->epoch(),
                                       request->version() + 1))) {
        request->set_epoch(node_server_->meta_info_.version.epoch);
        auto iter = node_server_->meta_info_.table_info.find(task_info_.table_name);
        if (iter == node_server_->meta_info_.table_info.end()) {
          LOG(ERROR) << "Table " << task_info_.table_name << " should exist";
          return false;
        }
        request->set_version(iter->second.version());
      } else {
        usleep(500 * 1000L);
      }
    } else {
      LOG(ERROR) << "Request to dst " << task_info_.dst.ToString()
                 << " error " << response->DebugString();
      usleep(500 * 1000L);
    }
  } while (!brpc::IsAskedToQuit());
  return success;
}

bool NodeMigrate::CommitToMeta() {
  if (canceled_) {
    return false;
  }

  noah::meta::MetaCmdRequest request;
  noah::meta::MetaCmdResponse response;
  request.set_type(noah::meta::Type::MIGRATE);
  request.mutable_version()->set_epoch(node_server_->meta_info_.version.epoch);
  request.mutable_version()->set_version(node_server_->meta_info_.version.version);
  auto unit = request.mutable_migrate()->mutable_unit();
  task_info_.Convert2MigrateUnit(unit);
  do {
    if (!node_server_->meta_client_.MetaCmd(&request, &response,
          node_server_->options_.node_timeout_ms)) {
      usleep(500 * 1000L);
      continue;
    }
    if (noah::meta::Status::OK == response.status()) {
      node_server_->UpdateMetaInfo(VersionInfo(response.version()));
      break;
    } else if (noah::meta::Status::EEPOCH == response.status()) {
      if (node_server_->UpdateMetaInfo(VersionInfo(response.version()))) {
        request.mutable_version()->set_epoch(node_server_->meta_info_.version.epoch);
        request.mutable_version()->set_version(node_server_->meta_info_.version.version);
      } else {
        usleep(500 * 1000L);
      }
    } else {
      LOG(ERROR) << "Commit to MetaServer error " << response.DebugString();
      usleep(500 * 1000L);
    }
  } while (!brpc::IsAskedToQuit() && !canceled_);

  return true;
}

}  // namespace node
}  // namespace noah
