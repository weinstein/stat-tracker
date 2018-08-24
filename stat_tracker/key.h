#ifndef STAT_TRACKER_KEY_H_
#define STAT_TRACKER_KEY_H_

#include <string>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "leveldb/slice.h"

namespace stat_tracker {

class Key {
 public:
  static Key UserStatsPrefix(absl::string_view user_id);
  static Key NextStatId(absl::string_view user_id);
  static Key ForStat(absl::string_view user_id, absl::string_view stat_id);
  static bool ParseStat(absl::string_view key, std::string* user_id,
                        std::string* stat_id);

  static Key StatEventsPrefix(absl::string_view user_id,
                              absl::string_view stat_id);
  static Key NextEventId(absl::string_view user_id, absl::string_view stat_id);
  static Key ForEvent(absl::string_view user_id, absl::string_view stat_id,
                      absl::string_view event_id);

  static Key StatIndexPrefix(absl::string_view user_id,
                             absl::string_view stat_id);
  static Key IndexHitsPrefix(absl::string_view user_id,
                             absl::string_view stat_id,
                             absl::string_view token_id);
  static Key ForIndexHit(absl::string_view user_id, absl::string_view stat_id,
                         absl::string_view token_id,
                         absl::string_view event_id);

  operator absl::string_view() const { return data_; }
  operator leveldb::Slice() const { return leveldb::Slice(data_); }
  operator const std::string&() const { return data_; }

 private:
  explicit Key(std::string data) : data_(std::move(data)) {}

  std::string data_;
};

}  // namespace stat_tracker

#endif  // STAT_TRACKER_KEY_H_
