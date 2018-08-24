def gen_grpc_cc(name, src, deps=[],
                protoc="@com_google_protobuf_cc//:protoc",
                grpc="@com_github_grpc_grpc",
                **kwargs):
  basename, ext = src.rsplit('.', 1)
  if ext != 'proto':
    fail('only sources ending in .proto are supported', 'src')

  grpc_cpp_plugin = grpc + "//:grpc_cpp_plugin"

  path_to_protoc = "$(location %s)" % protoc
  path_to_plugin = "$(location %s)" % grpc_cpp_plugin
  path_to_src = "$(location %s)" % src

  command = (
      path_to_protoc +
      " --grpc_out=$(GENDIR)/ " +
      " --plugin=protoc-gen-grpc=" + path_to_plugin +
      " " + path_to_src
  )
  out_hdr = basename + '.grpc.pb.h'
  out_src = basename + '.grpc.pb.cc'
  grpc_outs = [out_hdr, out_src]

  native.genrule(
      name = name + "_genrule",
      srcs = [src],
      cmd = command,
      outs = grpc_outs,
      tools = [
          protoc,
          grpc_cpp_plugin,
      ],
  )

  grpc_lib = grpc + "//:grpc"
  native.cc_library(
      name = name,
      hdrs = [out_hdr],
      srcs = [out_src],
      deps = deps + [
          grpc_lib,
      ],
  )
