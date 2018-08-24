#include "stat_tracker/service_impl.h"

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/wrappers.pb.h"
#include "glog/logging.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"
#include "leveldb/write_batch.h"
#include "leveldb/status.h"
#include "stat_tracker/key.h"
#include "stat_tracker/time_util.h"
#include "storage/status_util.h"

namespace stat_tracker {

namespace {

std::string ToTokenString(absl::string_view prefix, const TimeRangeToken& token) {
  return absl::StrCat(prefix, "-", absl::FormatDuration(token.granularity), "@",
                      token.index);
}

template <typename T>
util::StatusOr<leveldb::Status, T> ProtoGet(leveldb::DB* db,
                                            const leveldb::ReadOptions& options,
                                            const leveldb::Slice& key) {
  std::string value_bytes;
  RETURN_IF_ERROR(db->Get(options, key, &value_bytes));

  T proto_value;
  if (!proto_value.ParseFromString(value_bytes)) {
    return leveldb::Status::Corruption(
        absl::StrCat("key ", key.ToString(), " not parseable."));
  }
  return std::move(proto_value);
}

util::StatusOr<leveldb::Status, std::string> SerializeAsString(
    const google::protobuf::Message& message) {
  std::string value_bytes;
  if (!message.SerializeToString(&value_bytes)) {
    return leveldb::Status::InvalidArgument("unserializable message");
  }
  return std::move(value_bytes);
}

leveldb::Status ProtoPut(const leveldb::Slice& key,
                         const google::protobuf::Message& value,
                         leveldb::WriteBatch* batch) {
  ASSIGN_OR_RETURN(std::string value_bytes, SerializeAsString(value));
  batch->Put(key, value_bytes);
  return leveldb::Status::OK();
}

}  // namespace

util::StatusOr<grpc::Status, util::LockMap<std::string>::Lock>
StatServiceImpl::AcquireUserLock(const grpc::ServerContext& context,
                                 const std::string& user_id) {
  auto user_lock =
      user_locks_.AcquireWithTimeout(user_id, TimeoutFromContext(context));
  if (!user_lock.has_value()) {
    return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                        "deadline exceeded while waiting for user lock");
  }
  return std::move(user_lock.value());
}

util::StatusOr<grpc::Status, uint64_t> StatServiceImpl::PostIncrement(
    const Key& key, leveldb::WriteBatch* batch) {
  auto value_or = ProtoGet<google::protobuf::UInt64Value>(
      leveldb_.get(), leveldb::ReadOptions(), key);
  if (value_or.status().IsNotFound()) {
    value_or = google::protobuf::UInt64Value();
  }
  RETURN_IF_ERROR(storage::ToGrpcStatus(value_or.status()));
  google::protobuf::UInt64Value& uint64_value = value_or.ValueOrDie();
  uint64_t value = uint64_value.value();
  uint64_value.set_value(value + 1);
  RETURN_IF_ERROR(storage::ToGrpcStatus(ProtoPut(key, uint64_value, batch)));
  return value;
}

grpc::Status StatServiceImpl::AppendEvent(const std::string& user_id,
                                          const Event& event,
                                          leveldb::WriteBatch* batch) {
  const Key stat_key = Key::ForStat(user_id, event.stat_id());
  RETURN_IF_ERROR(storage::ToGrpcStatus(
      ProtoGet<Stat>(leveldb_.get(), leveldb::ReadOptions(), stat_key)
          .status()));

  ASSIGN_OR_RETURN(
      const uint64_t event_id,
      PostIncrement(Key::NextEventId(user_id, event.stat_id()), batch));
  const std::string event_id_str = absl::StrCat(event_id);

  const Key key = Key::ForEvent(user_id, event.stat_id(), event_id_str);
  RETURN_IF_ERROR(storage::ToGrpcStatus(ProtoPut(key, event, batch)));

  const absl::Time start_time = FromProtoTimestamp(event.start_time());
  const absl::Time end_time = start_time + FromProtoDuration(event.duration());
  for (const TimeRangeToken& token :
       tokenizer_.TokenizeTimeRange(start_time, end_time)) {
    const Key hit_key = Key::ForIndexHit(
        user_id, event.stat_id(), ToTokenString("r", token), event_id_str);
    batch->Put(hit_key, event_id_str);
  }
  for (const TimeRangeToken& token : tokenizer_.TokenizeTimePoint(start_time)) {
    const Key hit_key = Key::ForIndexHit(
        user_id, event.stat_id(), ToTokenString("p", token), event_id_str);
    batch->Put(hit_key, event_id_str);
  }
  for (const TimeRangeToken& token : tokenizer_.TokenizeTimePoint(end_time)) {
    const Key hit_key = Key::ForIndexHit(
        user_id, event.stat_id(), ToTokenString("p", token), event_id_str);
    batch->Put(hit_key, event_id_str);
  }
  return grpc::Status::OK;
}

util::StatusOr<grpc::Status, std::string> StatServiceImpl::AppendStat(
    const std::string& user_id, const Stat& stat, leveldb::WriteBatch* batch) {
  ASSIGN_OR_RETURN(const uint64_t stat_id,
                   PostIncrement(Key::NextStatId(user_id), batch));
  std::string new_stat_id = absl::StrCat(stat_id);
  const Key key = Key::ForStat(user_id, new_stat_id);
  RETURN_IF_ERROR(storage::ToGrpcStatus(ProtoPut(key, stat, batch)));
  return std::move(new_stat_id);
}

grpc::Status StatServiceImpl::DefineStat(grpc::ServerContext* context,
                                         const DefineStatRequest* request,
                                         DefineStatResponse* response) {
  ASSIGN_OR_RETURN(auto l, AcquireUserLock(*context, request->user_id()));
  leveldb::WriteBatch batch;
  ASSIGN_OR_RETURN(const std::string new_stat_id,
                   AppendStat(request->user_id(), request->stat(), &batch));
  RETURN_IF_ERROR(
      storage::ToGrpcStatus(leveldb_->Write(leveldb::WriteOptions(), &batch)));
  response->set_new_stat_id(new_stat_id);
  LOG(INFO) << "DefineStat request: " << request->ShortDebugString()
            << " response: " << response->ShortDebugString();
  return grpc::Status::OK;
}

grpc::Status StatServiceImpl::ReadPrefix(
    const Key& key_prefix,
    const std::function<void(const leveldb::Slice& key,
                             const leveldb::Slice& value)>& on_row) {
  auto it = absl::WrapUnique(leveldb_->NewIterator(leveldb::ReadOptions()));
  for (it->Seek(key_prefix); it->Valid() && it->key().starts_with(key_prefix);
       it->Next()) {
    VLOG(1) << "prefix read for " << absl::string_view(key_prefix) << " | "
              << it->key().ToString() << ": " << it->value().ToString();
    on_row(it->key(), it->value());
  }
  return storage::ToGrpcStatus(it->status());
}

grpc::Status StatServiceImpl::DeletePrefix(const Key& key_prefix,
                                           leveldb::WriteBatch* batch) {
  return ReadPrefix(
      key_prefix, [&](const leveldb::Slice& key, const leveldb::Slice& value) {
        batch->Delete(key);
      });
}

grpc::Status StatServiceImpl::DeleteStat(grpc::ServerContext* context,
                                         const DeleteStatRequest* request,
                                         google::protobuf::Empty*) {
  ASSIGN_OR_RETURN(auto l, AcquireUserLock(*context, request->user_id()));
  leveldb::WriteBatch batch;
  batch.Delete(Key::ForStat(request->user_id(), request->stat_id()));

  const Key events_prefix =
      Key::StatEventsPrefix(request->user_id(), request->stat_id());
  RETURN_IF_ERROR(DeletePrefix(events_prefix, &batch));

  const Key index_prefix =
      Key::StatIndexPrefix(request->user_id(), request->stat_id());
  RETURN_IF_ERROR(DeletePrefix(index_prefix, &batch));

  RETURN_IF_ERROR(storage::ToGrpcStatus(
      leveldb_->Write(leveldb::WriteOptions(), &batch)));
  LOG(INFO) << "DeleteState request: " << request->ShortDebugString();
  return grpc::Status::OK;
}

grpc::Status StatServiceImpl::ReadStats(grpc::ServerContext* context,
                                        const ReadStatsRequest* request,
                                        ReadStatsResponse* response) {
  ASSIGN_OR_RETURN(auto l, AcquireUserLock(*context, request->user_id()));
  const Key prefix = Key::UserStatsPrefix(request->user_id());
  RETURN_IF_ERROR(ReadPrefix(
      prefix, [&](const leveldb::Slice& key, const leveldb::Slice& value) {
        std::string user_id, stat_id;
        Stat stat;
        if (Key::ParseStat(key.ToString(), &user_id, &stat_id) &&
            stat.ParseFromString(value.ToString())) {
          response->mutable_stats()->insert({stat_id, stat});
        }
      }));
  LOG(INFO) << "ReadStats request: " << request->ShortDebugString()
            << " response: " << response->ShortDebugString();
  return grpc::Status::OK;
}

grpc::Status StatServiceImpl::ReadEvents(grpc::ServerContext* context,
                                         const ReadEventsRequest* request,
                                         ReadEventsResponse* response) {
  ASSIGN_OR_RETURN(auto l, AcquireUserLock(*context, request->user_id()));

  const absl::Time requested_start_time =
      FromProtoTimestamp(request->start_time());
  const absl::Time requested_end_time =
      requested_start_time + FromProtoDuration(request->duration());

  std::set<std::string> event_id_hits;
  for (const TimeRangeToken& token :
       tokenizer_.TokenizeTimeRange(requested_start_time, requested_end_time)) {
    const Key hits_prefix = Key::IndexHitsPrefix(
        request->user_id(), request->stat_id(), ToTokenString("p", token));
    RETURN_IF_ERROR(ReadPrefix(hits_prefix, [&](const leveldb::Slice& key,
                                                const leveldb::Slice& value) {
      event_id_hits.insert(value.ToString());
    }));
  }
  for (const TimeRangeToken& token :
       tokenizer_.TokenizeTimePoint(requested_start_time)) {
    const Key hits_prefix = Key::IndexHitsPrefix(
        request->user_id(), request->stat_id(), ToTokenString("r", token));
    RETURN_IF_ERROR(ReadPrefix(hits_prefix, [&](const leveldb::Slice& key,
                                                const leveldb::Slice& value) {
      event_id_hits.insert(value.ToString());
    }));
  }

  for (absl::string_view event_id : event_id_hits) {
    const Key event_key =
        Key::ForEvent(request->user_id(), request->stat_id(), event_id);
    auto event_or =
        ProtoGet<Event>(leveldb_.get(), leveldb::ReadOptions(), event_key);
    RETURN_IF_ERROR(storage::ToGrpcStatus(event_or.status()));
    response->mutable_events()->insert(
        {std::string(event_id), std::move(event_or.ValueOrDie())});
  }

  LOG(INFO) << "ReadEvents request: " << request->ShortDebugString()
            << " response: " << response->ShortDebugString();
  return grpc::Status::OK;
}

grpc::Status StatServiceImpl::RecordEvent(grpc::ServerContext* context,
                                          const RecordEventRequest* request,
                                          google::protobuf::Empty*) {
  ASSIGN_OR_RETURN(auto l, AcquireUserLock(*context, request->user_id()));
  leveldb::WriteBatch batch;
  RETURN_IF_ERROR(AppendEvent(request->user_id(), request->event(), &batch));
  RETURN_IF_ERROR(storage::ToGrpcStatus(
      leveldb_->Write(leveldb::WriteOptions(), &batch)));
  LOG(INFO) << "RecordEvent request: " << request->ShortDebugString();
  return grpc::Status::OK;
}

grpc::Status StatServiceImpl::DeleteEvent(const std::string& user_id,
                                          const std::string& stat_id,
                                          const std::string& event_id,
                                          leveldb::WriteBatch* batch) {
  const Key primary_event_key = Key::ForEvent(user_id, stat_id, event_id);
  batch->Delete(primary_event_key);

  auto event_or = ProtoGet<Event>(leveldb_.get(), leveldb::ReadOptions(),
                                  primary_event_key);
  if (event_or.status().IsNotFound()) {
    return grpc::Status::OK;
  }
  RETURN_IF_ERROR(storage::ToGrpcStatus(event_or.status()));
  const Event& event = event_or.ValueOrDie();

  const absl::Time start_time = FromProtoTimestamp(event.start_time());
  const absl::Time end_time = start_time + FromProtoDuration(event.duration());
  for (const TimeRangeToken& token :
       tokenizer_.TokenizeTimeRange(start_time, end_time)) {
    const Key hit_key =
        Key::ForIndexHit(user_id, stat_id, ToTokenString("r", token), event_id);
    batch->Delete(hit_key);
  }
  for (const TimeRangeToken& token : tokenizer_.TokenizeTimePoint(start_time)) {
    const Key hit_key =
        Key::ForIndexHit(user_id, stat_id, ToTokenString("p", token), event_id);
    batch->Delete(hit_key);
  }
  for (const TimeRangeToken& token : tokenizer_.TokenizeTimePoint(end_time)) {
    const Key hit_key =
        Key::ForIndexHit(user_id, stat_id, ToTokenString("p", token), event_id);
    batch->Delete(hit_key);
  }

  return grpc::Status::OK;
}

grpc::Status StatServiceImpl::DeleteEvent(grpc::ServerContext* context,
                                          const DeleteEventRequest* request,
                                          google::protobuf::Empty*) {
  ASSIGN_OR_RETURN(auto l, AcquireUserLock(*context, request->user_id()));
  leveldb::WriteBatch batch;
  RETURN_IF_ERROR(DeleteEvent(request->user_id(), request->stat_id(),
                              request->event_id(), &batch));
  RETURN_IF_ERROR(storage::ToGrpcStatus(
      leveldb_->Write(leveldb::WriteOptions(), &batch)));
  LOG(INFO) << "DeleteEvent request: " << request->ShortDebugString();
  return grpc::Status::OK;
}

}  // namespace stat_tracker
