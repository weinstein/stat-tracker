#ifndef UTIL_STATUS_TEST_MACROS_H_
#define UTIL_STATUS_TEST_MACROS_H_

#include "googletest/include/gtest/gtest.h"

#define EXPECT_OK(status)                            \
  do {                                               \
    auto _result = (status);                         \
    EXPECT_TRUE(_result.ok()) << _result.ToString(); \
  } while (0)

#define ASSERT_OK(status)                            \
  do {                                               \
    auto _result = (status);                         \
    ASSERT_TRUE(_result.ok()) << _result.ToString(); \
  } while (0)

#define ASSERT_OK_AND_ASSIGN_IMPL2(counter, lhs, rhs) \
  auto _result_##counter = (rhs);                    \
  ASSERT_OK(_result_##counter.status());             \
  lhs = std::move(_result_##counter.ValueOrDie())

#define ASSERT_OK_AND_ASSIGN_IMPL1(counter, lhs, rhs) \
  ASSERT_OK_AND_ASSIGN_IMPL2(counter, lhs, rhs)

#define ASSERT_OK_AND_ASSIGN(lhs, rhs) \
  ASSERT_OK_AND_ASSIGN_IMPL1(__COUNTER__, lhs, rhs)

// ==== GRPC Status macros ====

#define EXPECT_GRPC_OK(status)                            \
  do {                                                    \
    auto _result = (status);                              \
    EXPECT_TRUE(_result.ok()) << _result.error_message(); \
  } while (0)

#define ASSERT_GRPC_OK(status)                            \
  do {                                                    \
    auto _result = (status);                              \
    ASSERT_TRUE(_result.ok()) << _result.error_message(); \
  } while (0)

#define ASSERT_GRPC_OK_AND_ASSIGN_IMPL2(counter, lhs, rhs) \
  auto _result_##counter = (rhs);                    \
  ASSERT_GRPC_OK(_result_##counter.status());             \
  lhs = std::move(_result_##counter.ValueOrDie())

#define ASSERT_GRPC_OK_AND_ASSIGN_IMPL1(counter, lhs, rhs) \
  ASSERT_GRPC_OK_AND_ASSIGN_IMPL2(counter, lhs, rhs)

#define ASSERT_GRPC_OK_AND_ASSIGN(lhs, rhs) \
  ASSERT_GRPC_OK_AND_ASSIGN_IMPL1(__COUNTER__, lhs, rhs)

#endif  // UTIL_STATUS_TEST_MACROS_H_
