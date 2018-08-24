#include "stat_tracker/time_util.h"

#include <time.h>

#include "absl/time/clock.h"

namespace stat_tracker {

absl::Duration TimeoutFromContext(const grpc::ServerContext& context) {
  return absl::FromChrono(context.deadline()) - absl::Now();
}

absl::Time FromProtoTimestamp(const google::protobuf::Timestamp& timestamp) {
  timespec spec;
  spec.tv_sec = timestamp.seconds();
  spec.tv_nsec = timestamp.nanos();
  return absl::TimeFromTimespec(spec);
}

absl::Duration FromProtoDuration(const google::protobuf::Duration& duration) {
  timespec spec;
  spec.tv_sec = duration.seconds();
  spec.tv_nsec = duration.nanos();
  return absl::DurationFromTimespec(spec);
}

google::protobuf::Timestamp ToProtoTimestamp(const absl::Time& time) {
  auto spec = absl::ToTimespec(time);
  google::protobuf::Timestamp timestamp;
  timestamp.set_seconds(spec.tv_sec);
  timestamp.set_nanos(spec.tv_nsec);
  return timestamp;
}

google::protobuf::Duration ToProtoDuration(const absl::Duration& time) {
  auto spec = absl::ToTimespec(time);
  google::protobuf::Duration duration;
  duration.set_seconds(spec.tv_sec);
  duration.set_nanos(spec.tv_nsec);
  return duration;
}

}  // namespace stat_tracker
