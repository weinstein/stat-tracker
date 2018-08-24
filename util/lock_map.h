#ifndef UTIL_LOCK_MAP_H_
#define UTIL_LOCK_MAP_H_

#include <unordered_set>
#include <utility>
#include <iostream>

#include "absl/debugging/symbolize.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"

namespace util {

template <typename Key>
class LockMap {
 public:
  class Lock {
   public:
    Lock(Lock&& other) : map_(other.map_), key_(std::move(other.key_)) {
      other.map_ = nullptr;
    }

    Lock& operator=(Lock&& other) {
      release();
      std::swap(map_, other.map_);
      std::swap(key_, other.key_);
      return *this;
    }

    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;

    ~Lock() { release(); }

   private:
    Lock(LockMap* map, const Key& key) : map_(map), key_(key) {}

    void release() {
      if (map_ == nullptr) return;
      map_->ReleaseKey(key_);
      map_ = nullptr;
    }

    LockMap<Key>* map_;
    Key key_;

    friend class LockMap;
  };

  LockMap() = default;

  void EnableDebugLog(const char* name) {
    mu_.EnableDebugLog(name);
  }

  Lock Acquire(const Key& key) {
    auto key_is_free = [this, &key]() {
      return locked_keys_.find(key) == locked_keys_.end();
    };
    mu_.LockWhen(absl::Condition(&key_is_free));
    locked_keys_.insert(key);
    mu_.Unlock();
    return Lock(this, key);
  }

  absl::optional<Lock> AcquireWithTimeout(const Key& key,
                                          absl::Duration timeout) {
    auto key_is_free = [this, &key]() {
      return locked_keys_.find(key) == locked_keys_.end();
    };
    if (mu_.LockWhenWithTimeout(absl::Condition(&key_is_free), timeout)) {
      locked_keys_.insert(key);
      mu_.Unlock();
      return absl::make_optional(Lock(this, key));
    }
    mu_.Unlock();
    return absl::nullopt;
  }

 private:
  void ReleaseKey(const Key& key) {
    absl::MutexLock l(&mu_);
    locked_keys_.erase(key);
  }

  absl::Mutex mu_;
  std::unordered_set<Key> locked_keys_;

  friend class Lock;
};

}  // namespace util

#endif  // UTIL_LOCK_MAP_H_
