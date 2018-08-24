#ifndef STAT_TRACKER_TIME_UTIL_H_
#define STAT_TRACKER_TIME_UTIL_H_

#include "absl/time/time.h"
#include "include/grpcpp/server_context.h"
#include "google/protobuf/duration.pb.h"
#include "google/protobuf/timestamp.pb.h"

namespace stat_tracker {

absl::Duration TimeoutFromContext(const grpc::ServerContext& context);

absl::Time FromProtoTimestamp(const google::protobuf::Timestamp& timestamp);
absl::Duration FromProtoDuration(const google::protobuf::Duration& duration);

google::protobuf::Timestamp ToProtoTimestamp(const absl::Time& time);
google::protobuf::Duration ToProtoDuration(const absl::Duration& duration);

}  // namespace stat_tracker

#endif  // STAT_TRACKER_TIME_UTIL_H_
