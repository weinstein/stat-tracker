local_repository(
    name = "com_github_gflags_gflags",
    path = "third_party/gflags",
)

local_repository(
    name = "com_google_googletest",
    path = "third_party/googletest",
)

local_repository(
    name = "com_google_absl",
    path = "third_party/abseil-cpp",
)

new_local_repository(
    name = "com_google_leveldb",
    path = "third_party/leveldb",
    build_file = "BUILD.leveldb",
)

local_repository(
    name = "com_google_protobuf",
    path = "third_party/protobuf",
)

local_repository(
    name = "com_google_protobuf_cc",
    path = "third_party/protobuf",
)

local_repository(
    name = "com_github_grpc_grpc",
    path = "third_party/grpc",
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()

local_repository(
    name = "com_google_glog",
    path = "third_party/glog",
)

local_repository(
    name = "com_google_benchmark",
    path = "third_party/benchmark",
)
