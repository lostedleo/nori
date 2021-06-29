// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_CLIENT_CLUSTER_H_
#define NOAH_CLIENT_CLUSTER_H_

#include "common/entity.h"

#include <vector>

#include "butil/synchronization/rw_mutex.h"
#include "butil/memory/scoped_ptr.h"
#include "brpc/parallel_channel.h"
#include "butil/at_exit.h"

#include "common/meta_client.h"
#include "common/channels.h"
#include "common/entity.h"

#include "client/options.h"
#include "client/iterator.h"

namespace noah {
namespace client {
class NoahClient;
class ClientWrapper;

struct RequestInfo;

typedef noah::common::Status          Status;
typedef noah::common::VersionInfo     VersionInfo;

class Cluster {
 public:
  typedef noah::common::MetaClient      MetaClient;
  typedef noah::common::MetaInfo        MetaInfo;
  typedef noah::common::Channels        Channels;

  explicit Cluster(const Options& options);
  Cluster(const std::string& meta_group, const std::string& meta_conf);
  virtual ~Cluster();

  bool Init();

  Status CreateTable(const std::string& table_name, int32_t partition_num,
                     int32_t duplicate_num, int64_t capacity = 0);
  Status Set(const std::string& table_name, const KeyValueMap& key_values,
             int32_t ttl = -1);
  Status Get(const std::string& table_name, const StringVector& keys,
             KeyValueMap* key_values);
  Status Delete(const std::string& table_name, const StringVector& keys);
  Iterator* NewIterator(const std::string& table_name);
  Status ListTableInfo(const std::string& table_name, std::vector<int32_t>* offset_info);
  Status ListTable(const std::string& table_name, const VectorSplitInfo& split_infos,
                   KeyValueMap* key_values, std::string* last_key = NULL);
  Status DropTable(const StringVector& tables);
  Status FlushTable(const std::string& table);

 private:
  friend NoahClient;
  friend ClientWrapper;

  Status BatchSet(const std::string& table_name, const KeyValueMap& key_values,
                  int32_t ttl = -1);
  Status BatchGet(const std::string& table_name, const StringVector& keys,
                  KeyValueMap* key_values);
  Status BatchDelete(const std::string& table_name, const StringVector& keys);
  Status InnerListTableInfo(const std::string& table_name, std::vector<int32_t>* offset_info);
  Status InnerListTable(const std::string& table_name, const VectorSplitInfo& split_infos,
                        KeyValueMap* key_values, std::string* last_key = NULL);

  bool UpdateMetaInfo(const VersionInfo& info);
  bool UpdateWithTable(const std::string& table_name);
  bool UpdateTableInfo(const std::vector<std::string>& tables, bool all = false);
  bool ExistTable(const std::string& table_name, bool updated = false);

  bool FindTable(const std::string& table_name, RequestInfo* info);
  void SplitArrayRquest(const StringVector& keys, int32_t partition_num,
                        std::vector<StringVector>* split_keys);
  void SplitSetRequst(const KeyValueMap& key_values, int32_t partition_num,
                      std::vector<KeyValueMap>* split_result);

  static butil::AtExitManager g_exit_manager_;
  Options                     options_;
  scoped_ptr<Channels>        channels_;
  MetaClient                  meta_client_;
  butil::RWMutex              meta_lock_;
  MetaInfo                    meta_info_;
};

}  // namespace client
}  // namespace noah

#endif
