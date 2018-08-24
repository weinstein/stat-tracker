#ifndef STAT_TRACKER_TIME_INDEX_H_
#define STAT_TRACKER_TIME_INDEX_H_

#include <cstdint>
#include <functional>
#include <map>
#include <ostream>
#include <set>
#include <vector>

#include "absl/time/time.h"
#include "absl/types/span.h"

namespace stat_tracker {

struct TimeRangeToken {
  absl::Time start_time() const {
    return absl::UnixEpoch() + index * granularity;
  }
  absl::Time end_time() const {
    return absl::UnixEpoch() + (index + 1) * granularity;
  }

  int64_t index;
  absl::Duration granularity;
};

bool operator==(const TimeRangeToken& lhs, const TimeRangeToken& rhs);
bool operator<(const TimeRangeToken& lhs, const TimeRangeToken& rhs);
std::ostream& operator<<(std::ostream& os, const TimeRangeToken& rhs);

class IndexInterface {
 public:
  IndexInterface() = default;
  virtual ~IndexInterface() = default;

  virtual void AddItem(uint64_t item_id,
                       absl::Span<const TimeRangeToken> tokens) = 0;

  virtual void QueryUnion(
      absl::Span<const TimeRangeToken> tokens,
      const std::function<void(uint64_t)>& on_item) const = 0;
};

class InMemoryIndex : public IndexInterface {
 public:
  InMemoryIndex() = default;

  void AddItem(uint64_t item_id,
               absl::Span<const TimeRangeToken> tokens) override;

  void QueryUnion(absl::Span<const TimeRangeToken> tokens,
                  const std::function<void(uint64_t)>& on_item) const override;

 private:
  std::map<TimeRangeToken, std::vector<uint64_t>> hits_;
};

class Tokenizer {
 public:
  explicit Tokenizer(std::set<absl::Duration> granularities)
      : granularities_(std::move(granularities)) {}

  std::vector<TimeRangeToken> TokenizeTimePoint(absl::Time time_pt) const;

  std::vector<TimeRangeToken> TokenizeTimeRange(absl::Time start,
                                                absl::Time end) const;

 private:
  std::set<absl::Duration> granularities_;
};

}  // namespace stat_tracker

#endif  // STAT_TRACKER_TIME_INDEX_H_
