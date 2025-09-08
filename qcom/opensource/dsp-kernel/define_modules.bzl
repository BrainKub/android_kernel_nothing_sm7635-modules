# TODO
# Add ddk module definition for frpc-trusted driver
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")

load(
    "//build/kernel/kleaf:kernel.bzl",
    "ddk_headers",
    "ddk_module",
    "kernel_module",
    "kernel_modules_install",
    "kernel_module_group",
)

microxr_kernel_build = select({
    "//build/kernel/kleaf:microxr_kernel_build_true": True,
    "//build/kernel/kleaf:microxr_kernel_build_false": False,
})

def define_modules(target, variant):
    kernel_build_variant = "{}_{}".format(target, variant)

    # Path to dsp folder from msm-kernel/include/trace directory
    trace_include_path = "../../../{}/dsp".format(native.package_name())

    ddk_module(
        name = "{}_frpc-adsprpc".format(kernel_build_variant),
        kernel_build = select({
            "//build/kernel/kleaf:microxr_kernel_build_true": "//:target_kernel_build",
            "//build/kernel/kleaf:microxr_kernel_build_false": "//msm-kernel:{}".format(kernel_build_variant)
        }),
        deps = ["//msm-kernel:all_headers"],
        srcs = [
            "dsp/adsprpc.c",
            "dsp/adsprpc_compat.c",
            "dsp/adsprpc_compat.h",
            "dsp/adsprpc_rpmsg.c",
            "dsp/adsprpc_shared.h",
            "dsp/fastrpc_trace.h",
        ],
        local_defines = ["DSP_TRACE_INCLUDE_PATH={}".format(trace_include_path)],
        out = "frpc-adsprpc.ko",
        hdrs = [
            "include/linux/fastrpc.h",
            "include/uapi/fastrpc_shared.h",
        ],
        includes = [
            "include/linux",
            "include/uapi",
        ],
    )

    ddk_module(
        name = "{}_cdsp-loader".format(kernel_build_variant),
        kernel_build = select({
            "//build/kernel/kleaf:microxr_kernel_build_true": "//:target_kernel_build",
            "//build/kernel/kleaf:microxr_kernel_build_false": "//msm-kernel:{}".format(kernel_build_variant)
        }),
        deps = ["//msm-kernel:all_headers"],
        srcs = ["dsp/cdsp-loader.c"],
        out = "cdsp-loader.ko",
    )

    if microxr_kernel_build:
        kernel_module_group(
            name = "{}_modules".format(kernel_build_variant),
            srcs = [
                ":{}_frpc-adsprpc".format(kernel_build_variant),
                ":{}_cdsp-loader".format(kernel_build_variant),
            ],
        )

    copy_to_dist_dir(
        name = "{}_dsp-kernel_dist".format(kernel_build_variant),
        data = [
            ":{}_frpc-adsprpc".format(kernel_build_variant),
            ":{}_cdsp-loader".format(kernel_build_variant),
        ],
        dist_dir = "out/target/product/{}/dlkm/lib/modules/".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
    )
