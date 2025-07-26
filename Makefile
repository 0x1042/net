.PHONY: upgo buildgo buildcpp

upgo:
	bazel run //:gazelle-update-repos
	bazel run //:gazelle

buildgo:
	bazel build --config=opt --config=go //go:go

buildcpp:
	bazel build --config=opt --config=cpp //cpp:srv
