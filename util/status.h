#ifndef UTIL_STATUS_H_
#define UTIL_STATUS_H_

#include "absl/types/optional.h"

namespace util {

template <typename Status, typename Data>
class StatusOr {
 public:
  StatusOr(const Status& status) : status_(status) {}
  StatusOr(Status&& status) : status_(std::move(status)) {}

  StatusOr(const Data& data) : data_(data) {}
  StatusOr(Data&& data) : data_(std::move(data)) {}

  bool ok() const& { return status_.ok(); }
  const Status& status() const& { return status_; }
  Status& status() & { return status_; }

  const Data& ValueOrDie() const& { return data_.value(); }
  Data& ValueOrDie() & { return data_.value(); }
  Data&& ValueOrDie() && { return std::move(data_.value()); }

 private:
  Status status_;
  absl::optional<Data> data_;
};

}  // namespace util

#define RETURN_IF_ERROR(expr)         \
  do {                                \
    auto _expr_result = (expr);       \
    if (!_expr_result.ok()) {         \
      return std::move(_expr_result); \
    }                                 \
  } while (0)

#define ASSIGN_OR_RETURN_IMPL2(counter, lhs, rhs) \
  auto _result_##counter = (rhs);                 \
  RETURN_IF_ERROR(_result_##counter.status());    \
  lhs = std::move(_result_##counter.ValueOrDie())

#define ASSIGN_OR_RETURN_IMPL1(counter, lhs, rhs) \
  ASSIGN_OR_RETURN_IMPL2(counter, lhs, rhs)

#define ASSIGN_OR_RETURN(lhs, rhs) ASSIGN_OR_RETURN_IMPL1(__COUNTER__, lhs, rhs)

#endif  // UTIL_STATUS_H_
