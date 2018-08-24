#ifndef STORAGE_STATUS_UTIL_H_
#define STORAGE_STATUS_UTIL_H_

#include "include/grpcpp/grpcpp.h"
#include "leveldb/status.h"

namespace storage {

grpc::StatusCode ToGrpcStatusCode(const leveldb::Status& leveldb_status);

grpc::Status ToGrpcStatus(const leveldb::Status& leveldb_status);

}  // namespace storage

#endif  // STORAGE_STATUS_UTIL_H_
