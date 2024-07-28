package(
    default_visibility = [
        "//:__pkg__",
        "//:__subpackages__",
    ],
)

cc_library(
    name = "proxy",
    srcs = glob(["*.cc"]),
    hdrs = glob([
        "*.h",
    ]),
    includes = ["//lib"],
    strip_include_prefix = "//lib",
    deps = [
        "@asio",
        "@spdlog",
    ],
)
