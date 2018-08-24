#include "stat_tracker/time_index.h"

#include "absl/time/clock.h"
#include "googlemock/include/gmock/gmock.h"
#include "googletest/include/gtest/gtest.h"

namespace stat_tracker {
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::MockFunction;
using ::testing::StrictMock;

TEST(TimeRangeTokenTest, StartTime) {
  const TimeRangeToken token = {12, absl::Seconds(1)};
  EXPECT_EQ(token.start_time(), absl::UnixEpoch() + absl::Seconds(12));
}

TEST(TimeRangeTokenTest, EndTime) {
  const TimeRangeToken token = {12, absl::Seconds(1)};
  EXPECT_EQ(token.end_time(), absl::UnixEpoch() + absl::Seconds(13));
}

TEST(TimeRangeTokenTest, SetOfTokens) {
  const TimeRangeToken five_seconds = {5, absl::Seconds(1)},
                       three_seconds = {3, absl::Seconds(1)},
                       seven_seconds = {7, absl::Seconds(1)},
                       one_hour = {1, absl::Hours(1)};
  const std::set<TimeRangeToken> tokens = {five_seconds, three_seconds,
                                           seven_seconds, one_hour};
  EXPECT_THAT(tokens, ElementsAre(three_seconds, five_seconds, seven_seconds,
                                  one_hour));
}

TEST(TokenizerTest, TimePoint) {
  const Tokenizer tokenizer =
      Tokenizer({absl::Seconds(1), absl::Minutes(1), absl::Hours(1)});
  const absl::Time time_pt =
      absl::UnixEpoch() + absl::Minutes(2) + absl::Seconds(1);
  EXPECT_THAT(tokenizer.TokenizeTimePoint(time_pt),
              ElementsAre(TimeRangeToken{121, absl::Seconds(1)},
                          TimeRangeToken{2, absl::Minutes(1)},
                          TimeRangeToken{0, absl::Hours(1)}));
}

TEST(TokenizerTest, TimeRange) {
  const Tokenizer tokenizer =
      Tokenizer({absl::Seconds(1), absl::Minutes(1), absl::Minutes(2),
                 absl::Minutes(10), absl::Minutes(30), absl::Hours(1)});
  const absl::Time start_time = absl::UnixEpoch() + absl::Seconds(59);
  const absl::Time end_time =
      absl::UnixEpoch() + absl::Hours(3) + absl::Minutes(2) + absl::Seconds(1);
  EXPECT_THAT(tokenizer.TokenizeTimeRange(start_time, end_time),
              ElementsAre(TimeRangeToken{59, absl::Seconds(1)},
                          TimeRangeToken{1, absl::Minutes(1)},
                          TimeRangeToken{1, absl::Minutes(2)},
                          TimeRangeToken{2, absl::Minutes(2)},
                          TimeRangeToken{3, absl::Minutes(2)},
                          TimeRangeToken{4, absl::Minutes(2)},
                          TimeRangeToken{1, absl::Minutes(10)},
                          TimeRangeToken{2, absl::Minutes(10)},
                          TimeRangeToken{1, absl::Minutes(30)},
                          TimeRangeToken{1, absl::Hours(1)},
                          TimeRangeToken{2, absl::Hours(1)},
                          TimeRangeToken{90, absl::Minutes(2)},
                          TimeRangeToken{3 * 60 * 60 + 2 * 60 /*10920*/, absl::Seconds(1)}));
}

TEST(InMemoryIndexTest, QueryForPointsInRange) {
  const Tokenizer tokenizer =
      Tokenizer({absl::Seconds(1), absl::Minutes(1), absl::Minutes(2),
                 absl::Minutes(10), absl::Minutes(30), absl::Hours(1)});
  const absl::Time now = absl::Now();
  InMemoryIndex index;
  index.AddItem(1, tokenizer.TokenizeTimePoint(now + absl::Minutes(1)));
  index.AddItem(2, tokenizer.TokenizeTimePoint(now + absl::Minutes(2)));
  index.AddItem(3, tokenizer.TokenizeTimePoint(now + absl::Minutes(3)));

  StrictMock<MockFunction<void(uint64_t)>> on_item;
  EXPECT_CALL(on_item, Call(2));
  EXPECT_CALL(on_item, Call(3));
  index.QueryUnion(
      tokenizer.TokenizeTimeRange(now + absl::Minutes(1) + absl::Seconds(1),
                                  now + absl::Hours(1)),
      on_item.AsStdFunction());
}

TEST(InMemoryIndexTest, QueryForRangeContainingPoint) {
  const Tokenizer tokenizer =
      Tokenizer({absl::Seconds(1), absl::Minutes(1), absl::Minutes(2),
                 absl::Minutes(10), absl::Minutes(30), absl::Hours(1)});
  const absl::Time now = absl::Now();
  InMemoryIndex index;
  index.AddItem(1, tokenizer.TokenizeTimeRange(now + absl::Minutes(0),
                                               now + absl::Minutes(1)));
  index.AddItem(2, tokenizer.TokenizeTimeRange(now + absl::Minutes(1),
                                               now + absl::Minutes(2)));
  index.AddItem(3, tokenizer.TokenizeTimeRange(now + absl::Minutes(1),
                                               now + absl::Minutes(3)));

  StrictMock<MockFunction<void(uint64_t)>> on_item;
  EXPECT_CALL(on_item, Call(2));
  EXPECT_CALL(on_item, Call(3));
  index.QueryUnion(
      tokenizer.TokenizeTimePoint(now + absl::Minutes(1) + absl::Seconds(23)),
      on_item.AsStdFunction());
}

TEST(InMemoryIndexTest, QueryForPointsIsEmpty) {
  const Tokenizer tokenizer =
      Tokenizer({absl::Seconds(1), absl::Minutes(1), absl::Minutes(2),
                 absl::Minutes(10), absl::Minutes(30), absl::Hours(1)});
  const absl::Time now = absl::Now();
  InMemoryIndex index;
  index.AddItem(1, tokenizer.TokenizeTimePoint(now + absl::Minutes(1)));
  index.AddItem(2, tokenizer.TokenizeTimePoint(now + absl::Minutes(2)));
  index.AddItem(3, tokenizer.TokenizeTimePoint(now + absl::Minutes(3)));

  StrictMock<MockFunction<void(uint64_t)>> on_item;
  EXPECT_CALL(on_item, Call(_)).Times(0);
  index.QueryUnion(
      tokenizer.TokenizeTimeRange(now + absl::Minutes(1) + absl::Seconds(1),
                                  now + absl::Minutes(2) - absl::Seconds(1)),
      on_item.AsStdFunction());
}

TEST(InMemoryIndexTest, QueryForRangesIsEmpty) {
  const Tokenizer tokenizer =
      Tokenizer({absl::Seconds(1), absl::Minutes(1), absl::Minutes(2),
                 absl::Minutes(10), absl::Minutes(30), absl::Hours(1)});
  const absl::Time now = absl::Now();
  InMemoryIndex index;
  index.AddItem(1, tokenizer.TokenizeTimeRange(now + absl::Minutes(0),
                                               now + absl::Minutes(1)));
  index.AddItem(2, tokenizer.TokenizeTimeRange(now + absl::Minutes(1),
                                               now + absl::Minutes(2)));
  index.AddItem(3, tokenizer.TokenizeTimeRange(now + absl::Minutes(1),
                                               now + absl::Minutes(3)));

  StrictMock<MockFunction<void(uint64_t)>> on_item;
  EXPECT_CALL(on_item, Call(_)).Times(0);
  index.QueryUnion(
      tokenizer.TokenizeTimePoint(now + absl::Minutes(3) + absl::Seconds(1)),
      on_item.AsStdFunction());
}

}  // namespace
}  // namespace stat_tracker
