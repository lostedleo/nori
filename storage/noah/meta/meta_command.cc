// auth Zhenwei Zhu losted.leo@gmail.com

#include "meta/meta_command.h"

#include <gflags/gflags.h>

#include "butil/logging.h"

#include "meta/meta_info.h"
#include "meta/meta_server.h"

DECLARE_bool(log_applied_task);

namespace noah {
namespace meta {
MetaCommand::MetaCommand(MetaServer* meta_server)
  : meta_server_(meta_server)  {
  meta_migrate_ = &meta_server_->meta_migrate_;
  meta_info_ = &meta_server_->meta_info_;
}

MetaCommand::~MetaCommand() {
}

bool MetaCommand::PreDealRequest(const MetaCmdRequest* request,
                                MetaCmdResponse* response) {
  bool predeal = true;
  switch (request->type()) {
    case Type::PULL:
      DealPull(request, response);
      break;
    case Type::LISTTABLE:
      DealListTable(request, response);
      break;
    case Type::LISTNODE:
      DealListNode(request, response);
      break;
    case Type::LISTMETA:
      DealListMeta(request, response);
      break;
    default:
      predeal = false;
  }
  if (predeal) {
    LOG_IF(INFO, FLAGS_log_applied_task) << "PreDealRequest " << request->DebugString()
           << " response " << response->DebugString();
  } else {
    LOG_IF(INFO, FLAGS_log_applied_task) << "This request should apply to taft "
           << request->DebugString();
  }
  return predeal;
}

void MetaCommand::DealRequest(const MetaCmdRequest* request,
    MetaCmdResponse* response) {
  switch (request->type()) {
    case Type::PING:
      DealPing(request, response);
      break;
    case Type::INIT:
      DealInit(request, response);
      break;
    case Type::DROPTABLE:
      DealDropTable(request, response);
      break;
    case Type::DOWNNODES:
      DealDownNodes(request, response);
      break;
    case Type::MIGRATE:
      DealMigrate(request, response);
      break;
    default:
      if (response) {
        response->set_status(Status::NOTSUPPORT);
      }
      LOG(WARNING) << "Not support this type " << request->type();
      break;
  }
}

MetaInfoSnapshotPtr MetaCommand::GetMetaInfoSnapshot() {
  {
    butil::ReaderAutoLock lock_r(&lock_);
    if (snapshot_.get() && !meta_info_->Changed(snapshot_->version)) {
      return snapshot_;
    }
  }
  butil::WriterAutoLock lock_w(&lock_);
  if (snapshot_.get() && !meta_info_->Changed(snapshot_->version)) {
    return snapshot_;
  }
  snapshot_.reset(new MetaInfoSnapshot);
  meta_info_->GetSnapshot(snapshot_.get());
  return snapshot_;
}

bool MetaCommand::CheckEpoch(const VersionInfo& info, const MetaCmdRequest* request,
                          MetaCmdResponse* response) {
  if (request->type() == Type::PING || request->type() == Type::PULL) {
    if (info.epoch < request->version().epoch()) {
      LOG(WARNING) << "CheckEpoch error meta epoch " << info.epoch << " request epoch "
                   << request->version().epoch();
    }
    return true;
  }

  if (info.epoch != request->version().epoch()) {
    LOG(ERROR) << "CheckEpoch error meta epoch " << info.epoch << " request epoch "
               << request->version().epoch();
    if (response) {
      response->set_status(Status::EEPOCH);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
    return false;
  }

  // Init must epcoh and version is equal to meta_info
  if (request->type() == Type::INIT) {
    if (info.version != request->version().version()) {
      response->set_status(Status::EEPOCH);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  }
  return true;
}

void MetaCommand::DealPing(const MetaCmdRequest* request,
                           MetaCmdResponse* response) {
  MetaInfoSnapshotPtr snapshot = GetMetaInfoSnapshot();
  if (!CheckEpoch(snapshot->version, request, response)) {
    return;
  }

  VersionInfo info = meta_info_->UpdateNode(request->ping().node());
  Processor processor;
  if (meta_migrate_->HasMigrateTask()) {
    MetaInfoSnapshotPtr snapshot = GetMetaInfoSnapshot();
    if (meta_migrate_->GetMigrateTask(request->ping().node(), snapshot.get(),
          &processor)) {
      if (response) {
        auto migrate = response->mutable_ping()->mutable_migrate();
        migrate->set_name(processor.task.table_name);
        migrate->set_id(processor.task.partition_id);
        *migrate->mutable_fault() = processor.task.fault_node.node;
        *migrate->mutable_src() = processor.src;
        *migrate->mutable_dst() = processor.dst;
        migrate->set_role(processor.task.fault_node.role);
      }
      LOG(INFO) << "Migtate task " << processor.ToString();
    }
  }
  if (response) {
    response->set_status(Status::OK);
    response->mutable_version()->set_epoch(info.epoch);
    response->mutable_version()->set_version(info.version);
  }
}

void MetaCommand::DealPull(const MetaCmdRequest* request, MetaCmdResponse* response) {
  MetaInfoSnapshotPtr snapshot = GetMetaInfoSnapshot();
  if (!CheckEpoch(snapshot->version, request, response)) {
    return;
  }

  response->mutable_version()->set_epoch(snapshot->version.epoch);
  response->mutable_version()->set_version(snapshot->version.version);
  if (request->pull().has_node()) {
    std::string node_str = Node2String(request->pull().node());
    const std::set<std::string>& tables = snapshot->node_table[node_str];
    for (const auto& table : tables) {
      if (snapshot->table_info.find(table) != snapshot->table_info.end()) {
        response->mutable_pull()->add_info()->CopyFrom(snapshot->table_info[table]);
      }
    }
  } else if (request->pull().names_size()) {
    for (const auto& name : request->pull().names()) {
      if (snapshot->table_info.find(name) != snapshot->table_info.end()) {
        response->mutable_pull()->add_info()->CopyFrom(snapshot->table_info[name]);
      }
    }
  } else {
    if (request->pull().all()) {
      for (const auto& value : snapshot->table_info) {
        response->mutable_pull()->add_info()->CopyFrom(value.second);
      }
    }
  }
  for (const auto& value : snapshot->nodes) {
    auto node_status = response->mutable_pull()->add_node_status();
    String2Node(value.first, node_status->mutable_node());
    node_status->set_status(value.second.last_alive_time == 0? NodeState::DOWN:NodeState::UP);
  }
  response->set_status(Status::OK);
}

void MetaCommand::DealInit(const MetaCmdRequest* request, MetaCmdResponse* response) {
  VersionInfo request_info(request->version());
  VersionInfo info = meta_info_->CreateTable(request_info, request->init().table());
  if (response) {
    if (info.epoch > 0) {
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
      response->set_status(Status::OK);
    } else if (0 == info.epoch) {
      response->set_status(Status::EXIST);
    } else {
      response->set_status(Status::EEPOCH);
    }
  }
}

void MetaCommand::DealListTable(const MetaCmdRequest* request,
                                MetaCmdResponse* response) {
  MetaInfoSnapshotPtr snapshot = GetMetaInfoSnapshot();
  if (!CheckEpoch(snapshot->version, request, response)) {
    return;
  }

  response->mutable_version()->set_epoch(snapshot->version.epoch);
  response->mutable_version()->set_version(snapshot->version.version);
  response->set_status(Status::OK);
  for (const auto& value : snapshot->table_info) {
    response->mutable_list_table()->add_tables(value.first);
  }
}

void MetaCommand::DealListNode(const MetaCmdRequest* request,
                               MetaCmdResponse* response) {
  MetaInfoSnapshotPtr snapshot = GetMetaInfoSnapshot();
  if (!CheckEpoch(snapshot->version, request, response)) {
    return;
  }

  response->mutable_version()->set_epoch(snapshot->version.epoch);
  response->mutable_version()->set_version(snapshot->version.version);
  response->set_status(Status::OK);
  for (const auto& value : snapshot->nodes) {
    NodeStatus* node = response->mutable_list_node()->mutable_nodes()->Add();
    String2Node(value.first, node->mutable_node());
    if (value.second.last_alive_time) {
      node->set_status(NodeState::UP);
    } else {
      node->set_status(NodeState::DOWN);
    }
  }
}

void MetaCommand::DealListMeta(const MetaCmdRequest* request,
                               MetaCmdResponse* response) {
  MetaInfoSnapshotPtr snapshot = GetMetaInfoSnapshot();
  response->mutable_version()->set_epoch(snapshot->version.epoch);
  response->mutable_version()->set_version(snapshot->version.version);
  if (meta_server_->ListMeta(response->mutable_list_meta()->mutable_nodes())) {
    response->set_status(Status::OK);
  } else {
    response->set_status(Status::ERROR);
  }
}

void MetaCommand::DealDropTable(const MetaCmdRequest* request,
                                MetaCmdResponse* response) {
  StringVector tables;
  for (const auto& table : request->drop_table().names()) {
    tables.push_back(table);
  }
  VersionInfo request_info(request->version());
  VersionInfo info = meta_info_->DropTable(request_info, tables);
  if (info.epoch) {
    if (response) {
      response->set_status(Status::OK);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  } else {
    if (response) {
      info = meta_info_->version();
      response->set_status(Status::EEPOCH);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  }
}

void MetaCommand::DealDownNodes(const MetaCmdRequest* request,
                                MetaCmdResponse* response) {
  std::vector<Node> nodes;
  for (const auto& node : request->down_nodes().nodes()) {
    nodes.push_back(node);
  }
  MigrateTaskArray tasks;
  VersionInfo info = meta_info_->DownNodes(request->version().epoch(), nodes, &tasks);
  if (info.epoch) {
    meta_migrate_->AddMigrateTasks(tasks);
    if (response) {
      response->set_status(Status::OK);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  } else {
    if (response) {
      info = meta_info_->version();
      response->set_status(Status::EEPOCH);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  }
}

void MetaCommand::DealMigrate(const MetaCmdRequest* request,
                              MetaCmdResponse* response) {
  Processor processor(request->migrate().unit());
  VersionInfo request_info(request->version());
  VersionInfo info = meta_info_->CommitMigrate(request_info, processor);
  if (info.epoch) {
    meta_migrate_->CommitMigrate(processor);
    if (response) {
      response->set_status(Status::OK);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  } else {
    if (response) {
      info = meta_info_->version();
      response->set_status(Status::EEPOCH);
      response->mutable_version()->set_epoch(info.epoch);
      response->mutable_version()->set_version(info.version);
    }
  }
}
}  // namespace meta
}  // namespace noah

