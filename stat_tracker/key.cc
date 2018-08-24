#include "stat_tracker/key.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"

namespace stat_tracker {

Key Key::UserStatsPrefix(absl::string_view user_id) {
  return Key(absl::StrCat(user_id, " SD:"));
}

Key Key::NextStatId(absl::string_view user_id) {
  return Key(absl::StrCat(user_id, " next_stat"));
}

Key Key::ForStat(absl::string_view user_id, absl::string_view stat_id) {
  const std::string prefix = Key::UserStatsPrefix(user_id);
  return Key(absl::StrCat(prefix, stat_id));
}

bool Key::ParseStat(absl::string_view key, std::string* user_id,
                    std::string* stat_id) {
  std::pair<absl::string_view, absl::string_view> parts =
      absl::StrSplit(key, absl::MaxSplits(" SD:", 1));
  if (parts.first.empty() || parts.second.empty()) {
    return false;
  }
  *user_id = std::string(parts.first);
  *stat_id = std::string(parts.second);
  return true;
}

Key Key::StatEventsPrefix(absl::string_view user_id, absl::string_view stat_id) {
  return Key(absl::StrCat(user_id, " S:", stat_id));
}

Key Key::NextEventId(absl::string_view user_id, absl::string_view stat_id) {
  const std::string prefix = Key::StatEventsPrefix(user_id, stat_id);
  return Key(absl::StrCat(prefix, " next_event"));
}

Key Key::ForEvent(absl::string_view user_id, absl::string_view stat_id,
                  absl::string_view event_id) {
  const std::string prefix = Key::StatEventsPrefix(user_id, stat_id);
  return Key(absl::StrCat(prefix, " E:", event_id));
}

Key Key::StatIndexPrefix(absl::string_view user_id, absl::string_view stat_id) {
  return Key(absl::StrCat(user_id, " IS:", stat_id));
}

Key Key::IndexHitsPrefix(absl::string_view user_id, absl::string_view stat_id,
                         absl::string_view token_id) {
  const std::string prefix = Key::StatIndexPrefix(user_id, stat_id);
  return Key(absl::StrCat(prefix, " T:", token_id));
}

Key Key::ForIndexHit(absl::string_view user_id, absl::string_view stat_id,
                     absl::string_view token_id, absl::string_view event_id) {
  const std::string prefix = Key::IndexHitsPrefix(user_id, stat_id, token_id);
  return Key(absl::StrCat(prefix, " H:", event_id));
}

}  // namespace stat_tracker
