#ifndef STAT_TRACKER_SERVICE_IMPL_H_
#define STAT_TRACKER_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "google/protobuf/empty.pb.h"
#include "include/grpcpp/server_context.h"
#include "leveldb/db.h"
#include "stat_tracker/key.h"
#include "stat_tracker/service.grpc.pb.h"
#include "stat_tracker/service.pb.h"
#include "stat_tracker/time_index.h"
#include "util/lock_map.h"
#include "util/status.h"

namespace stat_tracker {

class StatServiceImpl final : public StatService::Service {
 public:
  struct Options {
    std::shared_ptr<leveldb::DB> db;
    std::set<absl::Duration> index_granularities;
  };
  explicit StatServiceImpl(const Options& options)
      : leveldb_(options.db), tokenizer_(options.index_granularities) {}

  grpc::Status DefineStat(grpc::ServerContext* context,
                          const DefineStatRequest* request,
                          DefineStatResponse* response) override;

  grpc::Status DeleteStat(grpc::ServerContext* context,
                          const DeleteStatRequest* request,
                          google::protobuf::Empty*) override;

  grpc::Status ReadStats(grpc::ServerContext* context,
                         const ReadStatsRequest* request,
                         ReadStatsResponse* response) override;

  grpc::Status ReadEvents(grpc::ServerContext* context,
                          const ReadEventsRequest* request,
                          ReadEventsResponse* response) override;

  grpc::Status RecordEvent(grpc::ServerContext* context,
                           const RecordEventRequest* request,
                           google::protobuf::Empty*) override;

  grpc::Status DeleteEvent(grpc::ServerContext* context,
                           const DeleteEventRequest* request,
                           google::protobuf::Empty*) override;

 private:
  util::StatusOr<grpc::Status, util::LockMap<std::string>::Lock>
  AcquireUserLock(const grpc::ServerContext& context,
                  const std::string& user_id);

  util::StatusOr<grpc::Status, uint64_t> PostIncrement(
      const Key& key, leveldb::WriteBatch* batch);

  grpc::Status AppendEvent(const std::string& user_id, const Event& event,
                           leveldb::WriteBatch* batch);
  grpc::Status DeleteEvent(const std::string& user_id,
                           const std::string& stat_id,
                           const std::string& event_id,
                           leveldb::WriteBatch* batch);

  util::StatusOr<grpc::Status, std::string> AppendStat(
      const std::string& user_id, const Stat& stat, leveldb::WriteBatch* batch);

  grpc::Status ReadPrefix(
      const Key& key_prefix,
      const std::function<void(const leveldb::Slice& key,
                               const leveldb::Slice& value)>& on_row);

  grpc::Status DeletePrefix(const Key& key_prefix, leveldb::WriteBatch* batch);

  util::LockMap<std::string> user_locks_;
  std::shared_ptr<leveldb::DB> leveldb_;
  const Tokenizer tokenizer_;
};

}  // namespace stat_tracker

#endif  // STAT_TRACKER_SERVICE_IMPL_H_
