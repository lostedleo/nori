licenses(["notice"])  # Apache v2

exports_files([
  "LICENSE",
  "CPPLINT.cfg"
])

load(":build_tools/bazel/brpc.bzl", "brpc_proto_library")

config_setting(
  name = "with_glog",
  define_values = {"with_glog": "true"},
  visibility = ["//visibility:public"],
)

config_setting(
  name = "unittest",
  define_values = {"unittest": "true"},
)

COPTS = [
  "-DBTHREAD_USE_FAST_PTHREAD_MUTEX",
  "-D__const__=",
  "-D_GNU_SOURCE",
  "-DUSE_SYMBOLIZE",
  "-DNO_TCMALLOC",
  "-D__STDC_FORMAT_MACROS",
  "-D__STDC_LIMIT_MACROS",
  "-D__STDC_CONSTANT_MACROS",
  "-DGFLAGS_NS=google",
] + select({
  ":with_glog": ["-DBRPC_WITH_GLOG=1"],
  "//conditions:default": ["-DBRPC_WITH_GLOG=0"],
})

LINKOPTS = [
  "-lpthread",
  "-lrt",
  "-lssl",
  "-lcrypto",
  "-ldl",
  "-lz",
]

genrule(
  name = "config_h",
  outs = [
    "net/butil/config.h",
  ],
  cmd = """cat << EOF  >$@""" + """
// This file is auto-generated.
#ifndef  BUTIL_CONFIG_H
#define  BUTIL_CONFIG_H

#ifdef BRPC_WITH_GLOG
#undef BRPC_WITH_GLOG
#endif
#define BRPC_WITH_GLOG """ + select({
  ":with_glog": "1",
  "//conditions:default": "0",
}) +
"""
#endif  // BUTIL_CONFIG_H
EOF
  """
)

BUTIL_SRCS = [
"net/butil/third_party/dmg_fp/g_fmt.cc",
  "net/butil/third_party/dmg_fp/dtoa_wrapper.cc",
  "net/butil/third_party/dynamic_annotations/dynamic_annotations.c",
  "net/butil/third_party/icu/icu_utf.cc",
  "net/butil/third_party/superfasthash/superfasthash.c",
  "net/butil/third_party/modp_b64/modp_b64.cc",
  "net/butil/third_party/nspr/prtime.cc",
  "net/butil/third_party/symbolize/demangle.cc",
  "net/butil/third_party/symbolize/symbolize.cc",
  "net/butil/third_party/snappy/snappy-sinksource.cc",
  "net/butil/third_party/snappy/snappy-stubs-internal.cc",
  "net/butil/third_party/snappy/snappy.cc",
  "net/butil/third_party/murmurhash3/murmurhash3.cpp",
  "net/butil/arena.cpp",
  "net/butil/at_exit.cc",
  "net/butil/atomicops_internals_x86_gcc.cc",
  "net/butil/base64.cc",
  "net/butil/big_endian.cc",
  "net/butil/cpu.cc",
  "net/butil/debug/alias.cc",
  "net/butil/debug/asan_invalid_access.cc",
  "net/butil/debug/crash_logging.cc",
  "net/butil/debug/debugger.cc",
  "net/butil/debug/debugger_posix.cc",
  "net/butil/debug/dump_without_crashing.cc",
  "net/butil/debug/proc_maps_linux.cc",
  "net/butil/debug/stack_trace.cc",
  "net/butil/debug/stack_trace_posix.cc",
  "net/butil/environment.cc",
  "net/butil/files/file.cc",
  "net/butil/files/file_posix.cc",
  "net/butil/files/file_enumerator.cc",
  "net/butil/files/file_enumerator_posix.cc",
  "net/butil/files/file_path.cc",
  "net/butil/files/file_path_constants.cc",
  "net/butil/files/memory_mapped_file.cc",
  "net/butil/files/memory_mapped_file_posix.cc",
  "net/butil/files/scoped_file.cc",
  "net/butil/files/scoped_temp_dir.cc",
  "net/butil/file_util.cc",
  "net/butil/file_util_linux.cc",
  "net/butil/file_util_posix.cc",
  "net/butil/guid.cc",
  "net/butil/guid_posix.cc",
  "net/butil/hash.cc",
  "net/butil/lazy_instance.cc",
  "net/butil/location.cc",
  "net/butil/md5.cc",
  "net/butil/memory/aligned_memory.cc",
  "net/butil/memory/ref_counted.cc",
  "net/butil/memory/ref_counted_memory.cc",
  "net/butil/memory/singleton.cc",
  "net/butil/memory/weak_ptr.cc",
  "net/butil/posix/file_descriptor_shuffle.cc",
  "net/butil/posix/global_descriptors.cc",
  "net/butil/rand_util.cc",
  "net/butil/rand_util_posix.cc",
  "net/butil/fast_rand.cpp",
  "net/butil/safe_strerror_posix.cc",
  "net/butil/sha1_portable.cc",
  "net/butil/strings/latin1_string_conversions.cc",
  "net/butil/strings/nullable_string16.cc",
  "net/butil/strings/safe_sprintf.cc",
  "net/butil/strings/string16.cc",
  "net/butil/strings/string_number_conversions.cc",
  "net/butil/strings/string_split.cc",
  "net/butil/strings/string_piece.cc",
  "net/butil/strings/string_util.cc",
  "net/butil/strings/string_util_constants.cc",
  "net/butil/strings/stringprintf.cc",
  "net/butil/strings/sys_string_conversions_posix.cc",
  "net/butil/strings/utf_offset_string_conversions.cc",
  "net/butil/strings/utf_string_conversion_utils.cc",
  "net/butil/strings/utf_string_conversions.cc",
  "net/butil/synchronization/cancellation_flag.cc",
  "net/butil/synchronization/condition_variable_posix.cc",
  "net/butil/synchronization/waitable_event_posix.cc",
  "net/butil/threading/non_thread_safe_impl.cc",
  "net/butil/threading/platform_thread_linux.cc",
  "net/butil/threading/platform_thread_posix.cc",
  "net/butil/threading/simple_thread.cc",
  "net/butil/threading/thread_checker_impl.cc",
  "net/butil/threading/thread_collision_warner.cc",
  "net/butil/threading/thread_id_name_manager.cc",
  "net/butil/threading/thread_local_posix.cc",
  "net/butil/threading/thread_local_storage.cc",
  "net/butil/threading/thread_local_storage_posix.cc",
  "net/butil/threading/thread_restrictions.cc",
  "net/butil/threading/watchdog.cc",
  "net/butil/time/clock.cc",
  "net/butil/time/default_clock.cc",
  "net/butil/time/default_tick_clock.cc",
  "net/butil/time/tick_clock.cc",
  "net/butil/time/time.cc",
  "net/butil/time/time_posix.cc",
  "net/butil/version.cc",
  "net/butil/logging.cc",
  "net/butil/class_name.cpp",
  "net/butil/errno.cpp",
  "net/butil/find_cstr.cpp",
  "net/butil/status.cpp",
  "net/butil/string_printf.cpp",
  "net/butil/thread_local.cpp",
  "net/butil/unix_socket.cpp",
  "net/butil/endpoint.cpp",
  "net/butil/fd_utility.cpp",
  "net/butil/files/temp_file.cpp",
  "net/butil/files/file_watcher.cpp",
  "net/butil/time.cpp",
  "net/butil/zero_copy_stream_as_streambuf.cpp",
  "net/butil/crc32c.cc",
  "net/butil/containers/case_ignored_flat_map.cpp",
  "net/butil/iobuf.cpp",
  "net/butil/popen.cpp",
]


cc_library(
  name = "butil",
  srcs = BUTIL_SRCS,
  hdrs = glob([
    "net/butil/*.h",
    "net/butil/*.hpp",
    "net/butil/**/*.h",
    "net/butil/**/*.hpp",
    "net/butil/**/**/*.h",
    "net/butil/**/**/*.hpp",
    "net/butil/**/**/**/*.h",
    "net/butil/**/**/**/*.hpp",
    "net/butil/third_party/dmg_fp/dtoa.cc",
    ]),
  deps = [
    "@com_google_protobuf//:protobuf",
    "@com_github_gflags_gflags//:gflags",
  ] + select({
    ":with_glog": ["@com_github_google_glog//:glog"],
    "//conditions:default": [],
  }),
  includes = [
    "net/",
  ],
  copts = COPTS + select({
    ":unittest": [
      "-DBVAR_NOT_LINK_DEFAULT_VARIABLES",
      "-DUNIT_TEST",
    ],
    "//conditions:default": [],
    }),
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

cc_library(
  name = "bvar",
  srcs = glob([
    "net/bvar/*.cpp",
    "net/bvar/detail/*.cpp",
  ],
  exclude = [
    "net/bvar/default_variables.cpp",
  ]) + select({
    ":unittest": [],
    "//conditions:default": ["net/bvar/default_variables.cpp"],
  }),
  hdrs = glob([
    "net/bvar/*.h",
    "net/bvar/utils/*.h",
    "net/bvar/detail/*.h",
  ]),
  includes = [
    "net/",
  ],
  deps = [
    ":butil",
  ],
  copts = COPTS + select({
    ":unittest": [
      "-DBVAR_NOT_LINK_DEFAULT_VARIABLES",
      "-DUNIT_TEST",
    ],
    "//conditions:default": [],
  }),
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

cc_library(
  name = "bthread",
  srcs = glob([
    "net/bthread/*.cpp",
  ]),
  hdrs = glob([
    "net/bthread/*.h",
    "net/bthread/*.list",
  ]),
  includes = [
    "net/"
  ],
  deps = [
    ":butil",
    ":bvar",
  ],
  copts = COPTS,
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

cc_library(
  name = "json2pb",
  srcs = glob([
    "net/json2pb/*.cpp",
  ]),
  hdrs = glob([
    "net/json2pb/*.h",
  ]),
  includes = [
    "net/",
  ],
  deps = [
    ":butil",
  ],
  copts = COPTS,
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

cc_library(
  name = "mcpack2pb",
  srcs = [
    "net/mcpack2pb/field_type.cpp",
    "net/mcpack2pb/mcpack2pb.cpp",
    "net/mcpack2pb/parser.cpp",
    "net/mcpack2pb/serializer.cpp",
  ],
  hdrs = glob([
    "net/mcpack2pb/*.h",
  ]),
  includes = [
    "net/",
  ],
  deps = [
    ":butil",
    ":cc_brpc_idl_options_proto",
    "@com_google_protobuf//:protoc_lib",
  ],
  copts = COPTS,
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

brpc_proto_library(
  name = "cc_brpc_idl_options_proto",
  srcs = [
    "net/idl_options.proto",
  ],
  deps = [
    "@com_google_protobuf//:cc_wkt_protos"
  ],
  visibility = ["//visibility:public"],
  )

brpc_proto_library(
  name = "cc_brpc_internal_proto",
  srcs = glob([
    "net/brpc/*.proto",
    "net/brpc/policy/*.proto",
  ]),
  include = "net/",
  deps = [
    ":cc_brpc_idl_options_proto",
    "@com_google_protobuf//:cc_wkt_protos"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "brpc",
  srcs = glob([
    "net/brpc/*.cpp",
    "net/brpc/**/*.cpp",
  ]),
  hdrs = glob([
    "net/brpc/*.h",
    "net/brpc/**/*.h"
  ]),
  includes = [
    "net/",
  ],
  deps = [
    ":butil",
    ":bthread",
    ":bvar",
    ":json2pb",
    ":mcpack2pb",
    ":cc_brpc_internal_proto",
    "@com_github_google_leveldb//:leveldb",
  ],
  copts = COPTS,
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "protoc-gen-mcpack",
  srcs = [
    "net/mcpack2pb/generator.cpp",
  ],
  deps = [
    ":cc_brpc_idl_options_proto",
    ":brpc",
  ],
  copts = COPTS,
  linkopts = LINKOPTS,
  visibility = ["//visibility:public"],
)

