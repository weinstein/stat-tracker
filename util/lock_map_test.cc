#include "util/lock_map.h"

#include <iostream>

#include "absl/time/time.h"
#include "googlemock/include/gmock/gmock.h"
#include "googletest/include/gtest/gtest.h"

namespace util {
namespace {

TEST(LockMapTest, AcquireFree) {
  LockMap<int> lock_map;
  EXPECT_TRUE(
      lock_map.AcquireWithTimeout(123, absl::ZeroDuration()).has_value());
}

TEST(LockMapTest, AcquireNotFree) {
  LockMap<int> lock_map;
  auto lock = lock_map.AcquireWithTimeout(123, absl::ZeroDuration());
  ASSERT_TRUE(lock.has_value());
  EXPECT_FALSE(lock_map.AcquireWithTimeout(123, absl::ZeroDuration()));
}

TEST(LockMapTest, AcquireFreeAfterRelease) {
  LockMap<int> lock_map;
  auto lock = lock_map.AcquireWithTimeout(123, absl::ZeroDuration());
  ASSERT_TRUE(lock.has_value());
  lock.reset();
  EXPECT_TRUE(
      lock_map.AcquireWithTimeout(123, absl::ZeroDuration()).has_value());
}

TEST(LockMapTest, MoveIntoLock) {
  LockMap<int> lock_map;
  auto lock = lock_map.AcquireWithTimeout(123, absl::ZeroDuration());
  ASSERT_TRUE(lock.has_value());

  lock = lock_map.AcquireWithTimeout(234, absl::ZeroDuration());
  EXPECT_TRUE(lock.has_value());
  EXPECT_TRUE(
      lock_map.AcquireWithTimeout(123, absl::ZeroDuration()).has_value());
  EXPECT_FALSE(
      lock_map.AcquireWithTimeout(234, absl::ZeroDuration()).has_value());
}

}  // namespace
}  // namespace util
