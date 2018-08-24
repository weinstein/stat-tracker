package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "status",
    hdrs = ["status.h"],
    deps = [
        "@com_google_absl//absl/types:optional",
    ],
)

cc_library(
    name = "status_test_macros",
    hdrs = ["status_test_macros.h"],
    deps = [
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "lock_map",
    hdrs = ["lock_map.h"],
    deps = [
        "@com_google_absl//absl/debugging:symbolize",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_test(
    name = "lock_map_test",
    srcs = ["lock_map_test.cc"],
    deps = [
        ":lock_map",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
