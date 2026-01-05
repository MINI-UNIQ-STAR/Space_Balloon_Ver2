Import("env")

import os
from SCons.Script import Action


def _remove_framework_cmsis_device_startup_conflict(build_env):
    """Avoid multiple-definition conflicts when providing a custom startup.

    The stm32cube framework builds and links libFrameworkCMSISDevice.a which
    contains the default startup (g_pfnVectors/Default_Handler). This project
    provides its own startup in Core/Src, so we drop the framework CMSIS Device
    library from the link step.
    """

    libs = list(build_env.get("LIBS", []))
    if "FrameworkCMSISDevice" in libs:
        build_env.Replace(LIBS=[lib for lib in libs if lib != "FrameworkCMSISDevice"])
        print("Custom build script: Removed FrameworkCMSISDevice to avoid startup symbol conflicts")


def _strip_framework_startup_object(build_env):
    """Remove the default startup object from the framework CMSIS archive.

    Some PlatformIO stm32cube configurations end up linking the framework
    startup unconditionally; removing the object from the archive avoids
    multiple-definition errors when the project supplies its own startup.
    """

    build_dir = build_env.subst("$BUILD_DIR")
    lib_path = os.path.join(build_dir, "libFrameworkCMSISDevice.a")
    ar = build_env.subst("$AR")
    cmd = f'{ar} d {lib_path} startup_stm32g431xx.o'

    build_env.AddPostAction(
        lib_path,
        Action(
            cmd,
            cmdstr="Custom build script: Stripping startup_stm32g431xx.o from libFrameworkCMSISDevice.a",
        ),
    )

# Add FPU flags to both compiler and linker
env.Append(
    CCFLAGS=[
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=hard"
    ],
    LINKFLAGS=[
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=hard"
    ]
)

_remove_framework_cmsis_device_startup_conflict(env)
_strip_framework_startup_object(env)

print("Custom build script: Added FPU flags to compiler and linker")
