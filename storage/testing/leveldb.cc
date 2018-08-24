#include "storage/testing/leveldb.h"

#include <stdlib.h>

#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "glog/logging.h"
#include "leveldb/options.h"

namespace storage {

namespace {

absl::string_view GetTestTmpDir() {
  return absl::string_view(std::getenv("TEST_TMPDIR"));
}

util::StatusOr<leveldb::Status, std::unique_ptr<leveldb::DB>> OpenTestDb(
    absl::string_view path) {
  leveldb::Options options;
  options.create_if_missing = true;

  LOG(INFO) << "leveldb path: " << path;
  leveldb::DB* db;
  RETURN_IF_ERROR(leveldb::DB::Open(options, std::string(path), &db));
  return absl::WrapUnique(db);
}

}  // namespace

LevelDbTestEnvironment::LevelDbTestEnvironment(absl::string_view relative_path)
    : path_(absl::StrCat(GetTestTmpDir(), "/", relative_path)),
      db_(OpenTestDb(path_).ValueOrDie()) {}

LevelDbTestEnvironment::~LevelDbTestEnvironment() {
  DumpContentsToInfoLogs();
  db_.reset();
  const auto status = leveldb::DestroyDB(path_, leveldb::Options());
  CHECK(status.ok()) << status.ToString();
}

void LevelDbTestEnvironment::DumpContentsToInfoLogs() {
  LOG(INFO) << "Dumping contents of " << path_;

  for (const std::string property : {"leveldb.stats", "leveldb.sstables",
                                     "leveldb.approximate-memory-usage"}) {
    std::string info_str;
    if (db_->GetProperty(property, &info_str)) {
      LOG(INFO) << property << ": " << info_str;
    }
  }

  auto it = absl::WrapUnique(db_->NewIterator(leveldb::ReadOptions()));
  if (it == nullptr) return;
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    LOG(INFO) << absl::CHexEscape(it->key().ToString()) << " = "
              << absl::CHexEscape(it->value().ToString()) << "\n";
  }
}

util::StatusOr<leveldb::Status, std::string> LevelDbTestEnvironment::Get(
    absl::string_view key) {
  std::string value;
  RETURN_IF_ERROR(db_->Get(leveldb::ReadOptions(),
                           leveldb::Slice(key.data(), key.size()), &value));
  return std::move(value);
}

leveldb::Status LevelDbTestEnvironment::Put(absl::string_view key,
                                            absl::string_view value) {
  leveldb::WriteOptions options;
  options.sync = true;
  return db_->Put(options, leveldb::Slice(key.data(), key.size()),
                  std::string(value));
}

leveldb::Status LevelDbTestEnvironment::Delete(absl::string_view key) {
  leveldb::WriteOptions options;
  options.sync = true;
  return db_->Delete(options, leveldb::Slice(key.data(), key.size()));
}

}  // namespace storage
