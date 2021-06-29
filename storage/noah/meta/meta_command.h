// auth Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_META_COMMAND_H_
#define NOAH_META_COMMAND_H_

#include <stdint.h>
#include <memory>

#include "butil/synchronization/rw_mutex.h"

namespace noah {
namespace meta {
class MetaMigrate;
class MetaInfo;
class MetaServer;
class MetaCmdRequest;
class MetaCmdResponse;
struct MetaInfoSnapshot;
struct VersionInfo;

typedef std::shared_ptr<MetaInfoSnapshot> MetaInfoSnapshotPtr;

class MetaCommand {
 public:
  explicit MetaCommand(MetaServer* meta_server);
  ~MetaCommand();

  bool PreDealRequest(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealRequest(const MetaCmdRequest* request, MetaCmdResponse* response);

 private:
  MetaInfoSnapshotPtr GetMetaInfoSnapshot();
  bool CheckEpoch(const VersionInfo& info, const MetaCmdRequest* request,
                  MetaCmdResponse* response);

  void DealPing(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealPull(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealInit(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealListTable(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealListNode(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealListMeta(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealDropTable(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealDownNodes(const MetaCmdRequest* request, MetaCmdResponse* response);
  void DealMigrate(const MetaCmdRequest* request, MetaCmdResponse* response);

  butil::RWMutex      lock_;
  MetaInfoSnapshotPtr snapshot_;
  MetaMigrate*        meta_migrate_;
  MetaInfo*           meta_info_;
  MetaServer*         meta_server_;
};

}  // namespace meta
}  // namespace noah

#endif
