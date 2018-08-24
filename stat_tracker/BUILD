package(
    default_visibility = ["//visibility:public"],
)

load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

cc_grpc_library(
    name = "service_cc_grpc_proto",
    srcs = ["service.proto"],
    deps = [],
    proto_only = False,
    well_known_protos = True,
    generate_mocks = True,
    use_external = True,
)

cc_library(
    name = "service_cc_proto",
    deps = [
        ":service_cc_grpc_proto",
        "@com_github_grpc_grpc//:grpc",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_library(
    name = "key",
    srcs = ["key.cc"],
    hdrs = ["key.h"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_leveldb//:leveldb",
    ],
)

cc_test(
    name = "key_test",
    srcs = ["key_test.cc"],
    deps = [
        ":key",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "time_util",
    srcs = ["time_util.cc"],
    hdrs = ["time_util.h"],
    deps = [
        "//proto:duration_cc_proto",
        "//proto:timestamp_cc_proto",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "service_impl",
    srcs = ["service_impl.cc"],
    hdrs = ["service_impl.h"],
    deps = [
      ":key",
      ":time_index",
      ":time_util",
      ":service_cc_proto",
      "//proto:empty_cc_proto",
      "//storage:status_util",
      "//util:lock_map",
      "//util:status",
      "@com_google_glog//:glog",
      "@com_google_leveldb//:leveldb",
    ],
)

cc_test(
    name = "service_impl_test",
    srcs = ["service_impl_test.cc"],
    deps = [
        ":service_impl",
        "//storage/testing:leveldb",
        "//util:status",
        "//util:status_test_macros",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_grpc_grpc//:grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "time_index",
    hdrs = ["time_index.h"],
    srcs = ["time_index.cc"],
    deps = [
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "time_index_test",
    srcs = ["time_index_test.cc"],
    deps = [
        ":time_index",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "time_index_benchmark",
    srcs = ["time_index_benchmark.cc"],
    deps = [
        ":time_index",
        "@com_google_absl//absl/time",
        "@com_google_benchmark//:benchmark",
        "@com_google_benchmark//:benchmark_main",
    ],
)

cc_binary(
    name = "service_main",
    srcs = ["service_main.cc"],
    deps = [
        ":service_impl",
        "//util:status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/memory",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_glog//:glog",
        "@com_github_grpc_grpc//:grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_leveldb//:leveldb",
    ],
)
