from pathlib import Path
from SCons.Script import *
from SCons.Variables import *
from SCons.Environment import *
from SCons.Node import *
from build_scripts.phony_targets import PhonyTargets
from build_scripts.utility import parse_size_dry, parse_size


VARS = Variables("build_scripts/config.py", ARGUMENTS)
VARS.AddVariables(
    EnumVariable(
        "config",
        help="Build configuration",
        default="test",
        allowed_values=["debug", "test", "release"],
    ),
    EnumVariable(
        "arch",
        help="Target architecture",
        default="i686",
        allowed_values=["i686"],
    ),
    EnumVariable(
        "image_filesystem",
        help="File system to use for the output image",
        default="fat32",
        allowed_values=["fat12", "fat16", "fat32"],
    ),
    BoolVariable(
        "display_commands",
        help="Display executed commands",
        default=False,
    ),
)

VARS.Add(
    "image_size",
    help="Size of the output image, will be rounded up to the next multiple of 512\n"
    "You can use suffixes: K, M, G",
    default="256m",
    converter=parse_size,
)
VARS.Add(
    "toolchain_path",
    help="Path to the toolchain",
    default=".toolchain",
)
VARS.Add(
    "memory_size",
    help="Size of the VM memory, this is ignored when building. You can use suffixes: K, M, G",
    default="64m",
    converter=parse_size_dry,
)

DEPENDANCIES = {
    "binutils": "2.44",
    "gcc": "11.4.0",
}

#
# Host environment (for tools and such)
#

HOST_ENVIRONMENT = Environment(
    variables=VARS,
    ENV=os.environ,
    AS="nasm",
    CC="gcc",
    CXX="g++",
    CFLAGS=["-std=c11"],  # only c
    CXXFLAGS=["-std=c++17"],  # only c++
    CCFLAGS=["-Wall", "-Werror"],  # both for c and c++
    STRIP="strip",
    SRC_DIRECTORY=str(Path("src").resolve()),
    BUILD_SCRIPTS_DIRECTORY=str(Path("build_scripts").resolve()),
    IMAGE_ROOT_DIRECTORY=str(Path("image/root").resolve()),
    IMAGE_GENERATED_ROOT_DIRECTORY=str(Path("image/generated_root").resolve()),
)

match HOST_ENVIRONMENT["config"]:
    case "debug":
        HOST_ENVIRONMENT.Append(CPPDEFINES={"DEBUG_BUILD": 1, "RELEASE_BUILD": 0})
        HOST_ENVIRONMENT.Append(CFLAGS=["-O0", "-g"])
    case "test":
        HOST_ENVIRONMENT.Append(CPPDEFINES={"DEBUG_BUILD": 0, "RELEASE_BUILD": 0})
        HOST_ENVIRONMENT.Append(CFLAGS=["-O2"])
    case "release":
        HOST_ENVIRONMENT.Append(CPPDEFINES={"DEBUG_BUILD": 0, "RELEASE_BUILD": 1})
        HOST_ENVIRONMENT.Append(CFLAGS=["-Ofast"])


if not HOST_ENVIRONMENT["display_commands"]:
    HOST_ENVIRONMENT.Replace(
        ASCOMSTR="Assembling [$SOURCE]",
        CCCOMSTR="Compiling  [$SOURCE]",
        CXXCOMSTR="Compiling  [$SOURCE]",
        FORTRANPPCOMSTR="Compiling  [$SOURCE]",
        FORTRANCOMSTR="Compiling  [$SOURCE]",
        SHCCCOMSTR="Compiling  [$SOURCE]",
        SHCXXCOMSTR="Compiling  [$SOURCE]",
        LINKCOMSTR="Linking    [$TARGET]",
        SHLINKCOMSTR="Linking    [$TARGET]",
        INSTALLSTR="Installing [$TARGET]",
        ARCOMSTR="Archiving  [$TARGET]",
        RANLIBCOMSTR="Ranlib     [$TARGET]",
    )

#
# Target environment
#

platform_prefix = ""
if HOST_ENVIRONMENT["arch"] == "i686":
    platform_prefix = "i686-elf-"

toolchainDir = Path(
    HOST_ENVIRONMENT["toolchain_path"], platform_prefix.removesuffix("-")
).resolve()
toolchainBin = Path(toolchainDir, "bin")
toolchainGccLibs = Path(
    toolchainDir, "lib", "gcc", platform_prefix.removesuffix("-"), DEPENDANCIES["gcc"]
)

TARGET_ENVIRONMENT = HOST_ENVIRONMENT.Clone(
    PLATFORM_PREFIX=platform_prefix,
    AR=f"{platform_prefix}ar",
    CC=f"{platform_prefix}gcc",
    CXX=f"{platform_prefix}g++",
    LD=f"{platform_prefix}g++",
    RANLIB=f"{platform_prefix}ranlib",
    STRIP=f"{platform_prefix}strip",
    TOOLCHAIN_PREFIX=str(toolchainDir),
    TOOLCHAIN_LIBGCC=str(toolchainGccLibs),
    BINUTILS_URL=f"https://ftp.gnu.org/gnu/binutils/binutils-{DEPENDANCIES['binutils']}.tar.xz",
    GCC_URL=f"https://ftp.gnu.org/gnu/gcc/gcc-{DEPENDANCIES['gcc']}/gcc-{DEPENDANCIES['gcc']}.tar.xz",
)

TARGET_ENVIRONMENT.Append(
    ASFLAGS=["-f", "elf", "-g"],
    CCFLAGS=["-ffreestanding", "-nostdlib"],
    CXXFLAGS=["-fno-exceptions", "-fno-rtti"],
    LINKFLAGS=["-nostdlib"],
    LIBS=["gcc"],
    LIBSPATH=[str(toolchainGccLibs)],
)

TARGET_ENVIRONMENT["ENV"]["PATH"] += os.pathsep + str(toolchainBin)

Help(VARS.GenerateHelpText(HOST_ENVIRONMENT))
Export("HOST_ENVIRONMENT")
Export("TARGET_ENVIRONMENT")

variant_dir = str(
    (
        Path("build") / f"{TARGET_ENVIRONMENT['arch']}_{TARGET_ENVIRONMENT['config']}"
    ).resolve()
)
variant_dir_shared_files = f"{variant_dir}/shared_files"
variant_dir_bootloader_stage_1 = (
    f"{variant_dir}/bootloader_stage_1_{TARGET_ENVIRONMENT['image_filesystem']}"
)
variant_dir_bootloader_stage_2 = f"{variant_dir}/bootloader_stage_2"
variant_dir_kernel = f"{variant_dir}/kernel"

# needs to run before including SConscripts because it "creates" source files
SConscript(
    "build_scripts/shared_files/SConscript",
    duplicate=0,
)
SConscript(
    "build_scripts/generate_isr/SConscript",
    duplicate=0,
)
Import("link_shared_files")
link_shared_files(TARGET_ENVIRONMENT)
Import("generate_isr")
generate_isr(f"{TARGET_ENVIRONMENT['SRC_DIRECTORY']}/kernel/arch/i686")

# for the os
SConscript(
    "build_scripts/fonts/SConscript",
    duplicate=0,
)
SConscript(
    "src/bootloader/stage_1/SConscript",
    variant_dir=variant_dir_bootloader_stage_1,
    duplicate=0,
)
SConscript(
    "src/bootloader/stage_2/SConscript",
    variant_dir=variant_dir_bootloader_stage_2,
    duplicate=0,
)
SConscript(
    "src/kernel/SConscript",
    variant_dir=variant_dir_kernel,
    duplicate=0,
)
SConscript(
    "image/SConscript",
    variant_dir=variant_dir,
    duplicate=0,
)

Import("kernel")
Import("disk_image")

#
# Phony targets, just like in make
#
PhonyTargets(
    HOST_ENVIRONMENT,
    run=[
        sys.executable,
        "scripts/run.py",
        disk_image[0].path,
        HOST_ENVIRONMENT["memory_size"],
    ],
    gdb=[
        sys.executable,
        "scripts/gdb.py",
        kernel[0].path,
        disk_image[0].path,
        HOST_ENVIRONMENT["memory_size"],
    ],
    toolchain=[
        sys.executable,
        "scripts/make-toolchain.py",
        TARGET_ENVIRONMENT["PLATFORM_PREFIX"],
        HOST_ENVIRONMENT["toolchain_path"],
        TARGET_ENVIRONMENT["TOOLCHAIN_PREFIX"],
        TARGET_ENVIRONMENT["BINUTILS_URL"],
        DEPENDANCIES["binutils"],
        TARGET_ENVIRONMENT["GCC_URL"],
        DEPENDANCIES["gcc"],
    ],
)

Depends("run", disk_image)
Depends("gdb", disk_image)
