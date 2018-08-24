package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "leveldb",
    srcs = ["leveldb.cc"],
    hdrs = ["leveldb.h"],
    deps = [
        "//util:status",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_leveldb//:leveldb",
    ],
)
