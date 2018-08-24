#include <memory>
#include <string>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "include/grpc/grpc.h"
#include "include/grpcpp/grpcpp.h"
#include "leveldb/db.h"
#include "leveldb/status.h"
#include "stat_tracker/service_impl.h"
#include "util/status.h"

DEFINE_int32(port, 8080, "port to run services on");
DEFINE_string(leveldb_path, "/dev/null",
              "path to leveldb where paxos entity metadata will be stored");

namespace {

util::StatusOr<leveldb::Status, std::unique_ptr<leveldb::DB>> OpenLevelDbUnique(
    const std::string& path) {
  LOG(INFO) << "opening leveldb path: " << path;
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  RETURN_IF_ERROR(leveldb::DB::Open(options, path, &db));
  return absl::WrapUnique(db);
}

}  // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto database_or = OpenLevelDbUnique(FLAGS_leveldb_path);
  CHECK(database_or.ok()) << database_or.status().ToString();

  stat_tracker::StatServiceImpl::Options options;
  options.db = std::move(database_or.ValueOrDie());
  options.index_granularities = {
      absl::Milliseconds(100), absl::Milliseconds(500), absl::Seconds(1),
      absl::Seconds(5),        absl::Seconds(10),       absl::Seconds(30),
      absl::Minutes(1),        absl::Minutes(5),        absl::Minutes(10),
      absl::Minutes(30),       absl::Hours(1),          absl::Hours(1e1),
      absl::Hours(1e2),        absl::Hours(1e3),        absl::Hours(1e4),
      absl::Hours(1e5),        absl::Hours(1e6),        absl::Hours(1e7),
      absl::Hours(1e8),        absl::Hours(1e9),        absl::Hours(1e10),
      absl::Hours(1e11),       absl::Hours(1e12)};
  stat_tracker::StatServiceImpl service_impl(options);

  const std::string host_port = absl::StrCat("0.0.0.0:", FLAGS_port);
  LOG(INFO) << "starting server: " << host_port;
  grpc::ServerBuilder server_builder;
  server_builder.AddListeningPort(host_port,
                                  grpc::InsecureServerCredentials());
  server_builder.RegisterService(&service_impl);
  auto server = server_builder.BuildAndStart();
  LOG(INFO) << "server started.";
  server->Wait();
  return 0;
}
