#include "storage/status_util.h"

namespace storage {

grpc::StatusCode ToGrpcStatusCode(const leveldb::Status& leveldb_status) {
  if (leveldb_status.ok()) return grpc::StatusCode::OK;
  if (leveldb_status.IsNotFound()) return grpc::StatusCode::NOT_FOUND;
  if (leveldb_status.IsCorruption()) return grpc::StatusCode::INTERNAL;
  if (leveldb_status.IsIOError()) return grpc::StatusCode::INTERNAL;
  if (leveldb_status.IsNotSupportedError())
    return grpc::StatusCode::UNIMPLEMENTED;
  if (leveldb_status.IsInvalidArgument())
    return grpc::StatusCode::INVALID_ARGUMENT;
  return grpc::StatusCode::UNKNOWN;
}

grpc::Status ToGrpcStatus(const leveldb::Status& leveldb_status) {
  return grpc::Status(ToGrpcStatusCode(leveldb_status),
                      leveldb_status.ToString());
}

}  // namespace storage
