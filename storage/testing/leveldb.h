#ifndef STORAGE_TESTING_LEVELDB_H_
#define STORAGE_TESTING_LEVELDB_H_

#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "leveldb/db.h"
#include "leveldb/status.h"
#include "util/status.h"

namespace storage {

class LevelDbTestEnvironment {
 public:
  explicit LevelDbTestEnvironment(absl::string_view relative_path);

  ~LevelDbTestEnvironment();

  void DumpContentsToInfoLogs();

  util::StatusOr<leveldb::Status, std::string> Get(absl::string_view key);
  leveldb::Status Put(absl::string_view key, absl::string_view value);
  leveldb::Status Delete(absl::string_view key);

  std::shared_ptr<leveldb::DB> db() const { return db_; }

 private:
  const std::string path_;
  std::shared_ptr<leveldb::DB> db_;
};

}  // namespace storage

#endif  // STORAGE_TESTING_LEVELDB_H_
