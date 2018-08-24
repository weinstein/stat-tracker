#include "stat_tracker/time_index.h"

#include <tuple>

#include "glog/logging.h"

namespace stat_tracker {

namespace {

TimeRangeToken GetTokenForTime(absl::Time time_pt, absl::Duration granularity) {
  const absl::Duration time_since_epoch = time_pt - absl::UnixEpoch();
  absl::Duration unused_remainder;
  const int64_t index =
      absl::IDivDuration(time_since_epoch, granularity, &unused_remainder);
  return {index, granularity};
}

}  // namespace

bool operator==(const TimeRangeToken& lhs, const TimeRangeToken& rhs) {
  return std::tie(lhs.granularity, lhs.index) ==
         std::tie(rhs.granularity, rhs.index);
}

bool operator<(const TimeRangeToken& lhs, const TimeRangeToken& rhs) {
  return std::tie(lhs.granularity, lhs.index) <
         std::tie(rhs.granularity, rhs.index);
}

std::ostream& operator<<(std::ostream& os, const TimeRangeToken& rhs) {
  os << "{" << rhs.index << "," << rhs.granularity << "}[" << rhs.start_time()
     << "," << rhs.end_time() << ")";
  return os;
}

void InMemoryIndex::AddItem(uint64_t item_id,
                            absl::Span<const TimeRangeToken> tokens) {
  for (const TimeRangeToken& token : tokens) {
    hits_[token].push_back(item_id);
  }
}

void InMemoryIndex::QueryUnion(
    absl::Span<const TimeRangeToken> tokens,
    const std::function<void(uint64_t)>& on_item) const {
  for (const TimeRangeToken& token : tokens) {
    auto it = hits_.find(token);
    if (it == hits_.end()) continue;
    for (uint64_t hit : it->second) {
      on_item(hit);
    }
  }
}

std::vector<TimeRangeToken> Tokenizer::TokenizeTimePoint(
    absl::Time time_pt) const {
  std::vector<TimeRangeToken> tokens;
  for (absl::Duration granularity : granularities_) {
    tokens.push_back(GetTokenForTime(time_pt, granularity));
  }
  return tokens;
}

std::vector<TimeRangeToken> Tokenizer::TokenizeTimeRange(
    absl::Time start, absl::Time end) const {
  if (start > end) std::swap(start, end);
  const absl::Duration finest_granularity = *granularities_.begin();
  start = GetTokenForTime(start, finest_granularity).start_time();
  end = GetTokenForTime(end, finest_granularity).start_time();

  std::vector<TimeRangeToken> tokens;
  while (start < end) {
    VLOG(1) << "tokenizing [" << start << "," << end << ")";
    TimeRangeToken next_token;
    for (auto it = granularities_.rbegin(); it != granularities_.rend(); ++it) {
      TimeRangeToken token = GetTokenForTime(start, *it);
      if (token.start_time() >= start && token.end_time() <= end) {
        next_token = std::move(token);
        break;
      }
      VLOG(1) << "token " << token << " didn't work.";
    }
    start = next_token.end_time();
    tokens.push_back(std::move(next_token));
  }
  return tokens;
}

}  // namespace stat_tracker
