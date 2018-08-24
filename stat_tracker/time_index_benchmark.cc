#include <cmath>
#include <set>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "benchmark/benchmark.h"
#include "glog/logging.h"
#include "stat_tracker/time_index.h"

namespace {

// growth ** n = coarsest / finest
// growth = (coarsest / finest) ** (1/n)
std::set<absl::Duration> GenerateGranularities(
    absl::Duration finest, absl::Duration coarsest, int n) {
  const double growth_factor = std::pow(absl::FDivDuration(coarsest, finest), 1. / n);

  std::set<absl::Duration> result;
  absl::Duration cur = finest;
  for (int i = 0; i < n; ++i) {
    VLOG(1) << "granularity " << cur;
    result.insert(cur);
    cur *= growth_factor;
  }
  return result;
}

}  // namespace

static void BM_IndexPoint(benchmark::State& state) {
  const stat_tracker::Tokenizer tokenizer =
      stat_tracker::Tokenizer(GenerateGranularities(
          absl::Seconds(1), 1000 * 365 * absl::Hours(24), state.range(0)));
  int num_tokens_generated = 0;
  const absl::Time now = absl::Now();
  for (auto _ : state) {
    num_tokens_generated += tokenizer.TokenizeTimePoint(now).size();
  }
  state.SetItemsProcessed(num_tokens_generated);
}
BENCHMARK(BM_IndexPoint)->DenseRange(4, 64, 4);

static void BM_IndexRange(benchmark::State& state) {
  const stat_tracker::Tokenizer tokenizer = stat_tracker::Tokenizer(
      {absl::Milliseconds(100), absl::Milliseconds(500), absl::Seconds(1),
       absl::Seconds(5), absl::Seconds(10), absl::Seconds(30), absl::Minutes(1),
       absl::Minutes(5), absl::Minutes(10), absl::Minutes(30), absl::Hours(1),
       absl::Hours(1e1), absl::Hours(1e2), absl::Hours(1e3), absl::Hours(1e4),
       absl::Hours(1e5), absl::Hours(1e6)});
  int num_tokens_generated = 0;
  const absl::Time now = absl::Now();
  for (auto _ : state) {
    num_tokens_generated +=
        tokenizer.TokenizeTimeRange(now - absl::Hours(state.range(0)), now)
            .size();
  }
  state.SetItemsProcessed(num_tokens_generated);
  state.counters["tokens_per_iter"] = num_tokens_generated / state.iterations();
}
BENCHMARK(BM_IndexRange)->Range(1, 1e6);
