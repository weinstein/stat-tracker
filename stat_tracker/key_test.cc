#include "stat_tracker/key.h"

#include <string>

#include "googlemock/include/gmock/gmock.h"
#include "googletest/include/gtest/gtest.h"

namespace stat_tracker {
namespace {

TEST(KeyTest, NextStatId) {
  const std::string key = Key::NextStatId("jack");
  EXPECT_EQ(key, "jack next_stat");
}

TEST(KeyTest, Stat) {
  const std::string key = Key::ForStat("jack", "foo_stat");
  EXPECT_EQ(key, "jack SD:foo_stat");
}

TEST(KeyTest, ParseStatOk) {
  std::string user_id, stat_id;
  EXPECT_TRUE(
      Key::ParseStat(Key::ForStat("jack", "foo_stat"), &user_id, &stat_id));
  EXPECT_EQ(user_id, "jack");
  EXPECT_EQ(stat_id, "foo_stat");
}

TEST(KeyTest, ParseBadStat) {
  std::string user_id, stat_id;
  EXPECT_FALSE(Key::ParseStat("asdf", &user_id, &stat_id));
  EXPECT_FALSE(Key::ParseStat("jack SD:", &user_id, &stat_id));
  EXPECT_FALSE(Key::ParseStat(Key::ForEvent("jack", "foo_stat", "bar_event"),
                              &user_id, &stat_id));
}

TEST(KeyTest, StatIdPrefix) {
  const std::string key = Key::StatEventsPrefix("jack", "foo_stat");
  EXPECT_EQ(key, "jack S:foo_stat");
}

TEST(KeyTest, NextEventId) {
  const std::string key = Key::NextEventId("jack", "foo_stat");
  EXPECT_EQ(key, "jack S:foo_stat next_event");
}

TEST(KeyTest, Event) {
  const std::string key = Key::ForEvent("jack", "foo_stat", "bar_event");
  EXPECT_EQ(key, "jack S:foo_stat E:bar_event");
}

}  // namespace
}  // namespace stat_tracker
