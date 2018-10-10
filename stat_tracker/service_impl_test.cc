#include "stat_tracker/service_impl.h"

#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "googlemock/include/gmock/gmock.h"
#include "googletest/include/gtest/gtest.h"
#include "include/grpc++/grpc++.h"
#include "include/grpc/grpc.h"
#include "stat_tracker/time_util.h"
#include "storage/testing/leveldb.h"
#include "util/status.h"
#include "util/status_test_macros.h"

DEFINE_int32(port, 8080, "");

namespace stat_tracker {
namespace {

using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Contains;
using ::testing::Not;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

std::set<absl::Duration> GenerateGranularities() {
  return {absl::Milliseconds(100), absl::Milliseconds(500), absl::Seconds(1),
          absl::Seconds(5),        absl::Seconds(10),       absl::Seconds(30),
          absl::Minutes(1),        absl::Minutes(5),        absl::Minutes(10),
          absl::Minutes(30),       absl::Hours(1),          absl::Hours(1e1),
          absl::Hours(1e2),        absl::Hours(1e3),        absl::Hours(1e4),
          absl::Hours(1e5),        absl::Hours(1e6),        absl::Hours(1e7),
          absl::Hours(1e8),        absl::Hours(1e9),        absl::Hours(1e10),
          absl::Hours(1e11),       absl::Hours(1e12)};
}

class ServiceImplTest : public ::testing::Test {
 protected:
  ServiceImplTest()
      : leveldb_env_("test.leveldb"),
        service_(StatServiceImpl::Options{leveldb_env_.db(),
                                          GenerateGranularities()}) {
    std::string host_port = absl::StrCat("localhost:", FLAGS_port);
    server_ =
        grpc::ServerBuilder()
            .AddListeningPort(host_port, grpc::InsecureServerCredentials())
            .RegisterService(&service_)
            .BuildAndStart();
    stub_ = StatService::NewStub(
        grpc::CreateChannel(host_port, grpc::InsecureChannelCredentials()));
  }

  template <typename Req, typename Resp>
  util::StatusOr<grpc::Status, Resp> Call(
      grpc::Status (StatService::Stub::*fn)(grpc::ClientContext*, const Req&,
                                            Resp*),
      const Req& req) {
    grpc::ClientContext ctx;
    Resp resp;
    RETURN_IF_ERROR((stub_.get()->*fn)(&ctx, req, &resp));
    return resp;
  }

  storage::LevelDbTestEnvironment leveldb_env_;

  StatServiceImpl service_;
  std::unique_ptr<grpc::Server> server_;
  std::unique_ptr<StatService::Stub> stub_;
};

TEST_F(ServiceImplTest, ReadEmptyStats) {
  ReadStatsRequest req;
  req.set_user_id("jack");
  ASSERT_GRPC_OK_AND_ASSIGN(ReadStatsResponse resp,
                            Call(&StatService::Stub::ReadStats, req));
  EXPECT_THAT(resp.stats(), IsEmpty());
}

TEST_F(ServiceImplTest, DefineStats) {
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  DefineStatRequest define_bar;
  define_bar.set_user_id("jack");
  define_bar.mutable_stat()->set_display_name("bar");
  ASSERT_GRPC_OK_AND_ASSIGN(DefineStatResponse bar_resp,
                            Call(&StatService::Stub::DefineStat, define_bar));
  const std::string bar_id = bar_resp.new_stat_id();
  EXPECT_NE(foo_id, bar_id);

  ReadStatsRequest read_req;
  read_req.set_user_id("jack");
  ASSERT_GRPC_OK_AND_ASSIGN(ReadStatsResponse read_resp,
                            Call(&StatService::Stub::ReadStats, read_req));

  auto display_name_eq = [](absl::string_view name) {
    return Property(&Stat::display_name, name);
  };
  EXPECT_THAT(read_resp.stats(),
              UnorderedElementsAre(Pair(foo_id, display_name_eq("foo")),
                                   Pair(bar_id, display_name_eq("bar"))));
}

TEST_F(ServiceImplTest, RecordEvents) {
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  RecordEventRequest event_req;
  event_req.set_user_id("jack");
  event_req.mutable_event()->set_stat_id(foo_id);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  ReadEventsRequest read_req;
  read_req.set_user_id("jack");
  read_req.add_stat_id(foo_id);
  *read_req.mutable_duration() = ToProtoDuration(absl::InfiniteDuration());
  ASSERT_GRPC_OK_AND_ASSIGN(ReadEventsResponse read_resp,
                            Call(&StatService::Stub::ReadEvents, read_req));
  EXPECT_THAT(read_resp.events_by_stat_id(), SizeIs(1))
      << read_resp.DebugString();
}

TEST_F(ServiceImplTest, ReadEventsOutsideOfTimeInterval) {
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  RecordEventRequest event_req;
  event_req.set_user_id("jack");
  event_req.mutable_event()->set_stat_id(foo_id);
  // Event spans [100, 150).
  event_req.mutable_event()->mutable_start_time()->set_seconds(100);
  event_req.mutable_event()->mutable_duration()->set_seconds(50);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  ReadEventsRequest read_req;
  read_req.set_user_id("jack");
  read_req.add_stat_id(foo_id);
  read_req.mutable_start_time()->set_seconds(151);
  read_req.mutable_duration()->set_seconds(1);
  ASSERT_GRPC_OK_AND_ASSIGN(ReadEventsResponse read_resp,
                            Call(&StatService::Stub::ReadEvents, read_req));
  EXPECT_THAT(read_resp.events_by_stat_id(), IsEmpty());
}

TEST_F(ServiceImplTest, ReadEventsInTimeInterval) {
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  RecordEventRequest event_req;
  event_req.set_user_id("jack");
  event_req.mutable_event()->set_stat_id(foo_id);
  // Event spans [100, 150).
  event_req.mutable_event()->mutable_start_time()->set_seconds(100);
  event_req.mutable_event()->mutable_duration()->set_seconds(50);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  // Event spans [140, 190).
  event_req.mutable_event()->mutable_start_time()->set_seconds(140);
  event_req.mutable_event()->mutable_duration()->set_seconds(50);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  // Event spans [60, 110).
  event_req.mutable_event()->mutable_start_time()->set_seconds(60);
  event_req.mutable_event()->mutable_duration()->set_seconds(50);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  // Event spans [120, 120).
  event_req.mutable_event()->mutable_start_time()->set_seconds(120);
  event_req.mutable_event()->mutable_duration()->set_seconds(0);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  ReadEventsRequest read_req;
  read_req.set_user_id("jack");
  read_req.add_stat_id(foo_id);
  // Read everything overlapping [105, 145).
  read_req.mutable_start_time()->set_seconds(105);
  read_req.mutable_duration()->set_seconds(40);
  ASSERT_GRPC_OK_AND_ASSIGN(ReadEventsResponse read_resp,
                            Call(&StatService::Stub::ReadEvents, read_req));
  EXPECT_THAT(read_resp.events_by_stat_id(),
              ElementsAre(Pair(
                  foo_id, Property(&ReadEventsResponse::Events::event_by_id,
                                   SizeIs(4)))));
}

TEST_F(ServiceImplTest, RecordManyEvents) {
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  constexpr int kNumEvents = 1000;
  const absl::Time now = absl::Now();
  for (int i = 0; i < kNumEvents; ++i) {
    RecordEventRequest event_req;
    event_req.set_user_id("jack");
    event_req.mutable_event()->set_stat_id(foo_id);
    *event_req.mutable_event()->mutable_start_time() =
        ToProtoTimestamp(now + absl::Hours(i) + absl::Seconds(23));
    *event_req.mutable_event()->mutable_duration() =
        ToProtoDuration(absl::Seconds(12));
    ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());
  }

  ReadEventsRequest read_req;
  read_req.set_user_id("jack");
  read_req.add_stat_id(foo_id);
  *read_req.mutable_start_time() = ToProtoTimestamp(now);
  *read_req.mutable_duration() = ToProtoDuration(absl::InfiniteDuration());
  ASSERT_GRPC_OK_AND_ASSIGN(ReadEventsResponse read_resp,
                            Call(&StatService::Stub::ReadEvents, read_req));
  EXPECT_THAT(read_resp.events_by_stat_id(),
              ElementsAre(Pair(
                  foo_id, Property(&ReadEventsResponse::Events::event_by_id,
                                   SizeIs(kNumEvents)))));
}

TEST_F(ServiceImplTest, DeleteStat) {
  // Create a stat called "foo".
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(const DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  // Record an event for it.
  RecordEventRequest event_req;
  event_req.set_user_id("jack");
  event_req.mutable_event()->set_stat_id(foo_id);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  // Delete the "foo" stat.
  DeleteStatRequest delete_foo;
  delete_foo.set_user_id("jack");
  delete_foo.set_stat_id(foo_id);
  ASSERT_GRPC_OK(Call(&StatService::Stub::DeleteStat, delete_foo).status());

  // Read the stats. No more "foo".
  ReadStatsRequest read_stats_req;
  read_stats_req.set_user_id("jack");
  ASSERT_GRPC_OK_AND_ASSIGN(
      const ReadStatsResponse read_stats_resp,
      Call(&StatService::Stub::ReadStats, read_stats_req));
  EXPECT_THAT(read_stats_resp.stats(), IsEmpty());

  // No more of foo's events either.
  ReadEventsRequest read_events_req;
  read_events_req.set_user_id("jack");
  read_events_req.add_stat_id(foo_id);
  *read_events_req.mutable_duration() =
      ToProtoDuration(absl::InfiniteDuration());
  ASSERT_GRPC_OK_AND_ASSIGN(
      const ReadEventsResponse read_events_resp,
      Call(&StatService::Stub::ReadEvents, read_events_req));
  EXPECT_THAT(read_events_resp.events_by_stat_id(), IsEmpty());
}

TEST_F(ServiceImplTest, DeleteEvent) {
  // Create a stat called "foo".
  DefineStatRequest define_foo;
  define_foo.set_user_id("jack");
  define_foo.mutable_stat()->set_display_name("foo");
  ASSERT_GRPC_OK_AND_ASSIGN(const DefineStatResponse foo_resp,
                            Call(&StatService::Stub::DefineStat, define_foo));
  const std::string foo_id = foo_resp.new_stat_id();

  // Record two events for it.
  RecordEventRequest event_req;
  event_req.set_user_id("jack");
  event_req.mutable_event()->set_stat_id(foo_id);
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());
  ASSERT_GRPC_OK(Call(&StatService::Stub::RecordEvent, event_req).status());

  // Read the two events.
  ReadEventsRequest read_events_req;
  read_events_req.set_user_id("jack");
  read_events_req.add_stat_id(foo_id);
  *read_events_req.mutable_duration() =
      ToProtoDuration(absl::InfiniteDuration());
  ASSERT_GRPC_OK_AND_ASSIGN(
      const ReadEventsResponse read_events_resp,
      Call(&StatService::Stub::ReadEvents, read_events_req));
  ASSERT_THAT(read_events_resp.events_by_stat_id(),
              ElementsAre(Pair(
                  foo_id, Property(&ReadEventsResponse::Events::event_by_id,
                                   SizeIs(2)))));

  // Delete the first one.
  const std::string event_to_delete = read_events_resp.events_by_stat_id()
                                          .at(foo_id)
                                          .event_by_id()
                                          .begin()
                                          ->first;
  DeleteEventRequest delete_req;
  delete_req.set_user_id("jack");
  delete_req.set_stat_id(foo_id);
  delete_req.set_event_id(event_to_delete);
  ASSERT_GRPC_OK(Call(&StatService::Stub::DeleteEvent, delete_req).status());

  // Only one event remains, and it's not the one we deleted.
  ASSERT_GRPC_OK_AND_ASSIGN(
      const ReadEventsResponse events_after_del,
      Call(&StatService::Stub::ReadEvents, read_events_req));
  EXPECT_THAT(
      events_after_del.events_by_stat_id(),
      ElementsAre(Pair(
          foo_id, Property(&ReadEventsResponse::Events::event_by_id,
                           AllOf(SizeIs(1),
                                 Not(Contains(Pair(event_to_delete, _))))))));
}

TEST_F(ServiceImplTest, DeleteNonExistentEvent) {
  DeleteEventRequest delete_req;
  delete_req.set_user_id("jack");
  delete_req.set_stat_id("foo");
  delete_req.set_event_id("asdf_doesnt_exist");
  ASSERT_GRPC_OK(Call(&StatService::Stub::DeleteEvent, delete_req).status());
}

TEST_F(ServiceImplTest, RecordEventForNonExistentStatIsError) {
  RecordEventRequest event_req;
  event_req.set_user_id("jack");
  event_req.mutable_event()->set_stat_id("asdf");
  const grpc::Status result =
      Call(&StatService::Stub::RecordEvent, event_req).status();
  EXPECT_EQ(result.error_code(), grpc::StatusCode::NOT_FOUND)
      << result.error_message();
}

}  // namespace
}  // namespace stat_tracker

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
