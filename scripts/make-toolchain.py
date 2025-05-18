#!/usr/bin/python3

import requests
import tarfile
import sys
import sh
import os


def chdir_wrapper(func, directory: str):
    def wrapper(*args):
        old_dir = os.getcwd()
        os.chdir(directory)
        func(*args)
        os.chdir(old_dir)

    return wrapper


def get_system_triplet() -> str:
    try:
        return sh.Command("gcc")("-dumpmachine").strip()  # type: ignore
    except Exception as error:
        print(f"Failed to get system triplet from GCC: {error}")
        return ""


def build_binutils(
    platform_prefix: str, toolchain_prefix: str, url: str, version: str
) -> None:
    print(f"Starting build of binutils {version} for {platform_prefix}...")

    print(f"Downloading binutils tarball from {url}...")
    response = requests.get(url)
    response.raise_for_status()

    tar_output_directory = os.path.abspath(f"binutils-{version}")
    build_directory = "binutils-build"
    tar_file = "binutils.tar.xz"

    with open(tar_file, "wb") as binutils_tar_fd:
        binutils_tar_fd.write(response.content)

    print("Extracting binutils tarball...")
    with tarfile.open(tar_file, "r:xz") as binutils_tar_fd:
        binutils_tar_fd.extractall(path=".")

    os.remove(tar_file)

    os.makedirs(build_directory, exist_ok=True)
    os.chdir(build_directory)
    print("Configuring binutils...")
    sh.Command(f"{tar_output_directory}/configure")(
        "--host",
        get_system_triplet(),
        "--build",
        get_system_triplet(),
        "--prefix",
        toolchain_prefix,
        "--target",
        platform_prefix,
        "--with-sysroot",
        "--disable-nls",
        "--disable-werror",
    )
    os.chdir("..")

    print("Building binutils...")
    make = sh.Command("make")
    make("-C", build_directory, "-j", str(os.cpu_count()))
    make("-C", build_directory, "install")


def build_gcc(
    platform_prefix: str, toolchain_prefix: str, url: str, version: str
) -> None:
    print(f"Starting build of GCC {version} for {platform_prefix}...")

    print(f"Downloading GCC tarball from {url}...")
    response = requests.get(url)
    response.raise_for_status()

    tar_output_directory = os.path.abspath(f"gcc-{version}")
    build_directory = "gcc-build"
    tar_file = "gcc.tar.xz"

    with open(tar_file, "wb") as binutils_tar_fd:
        binutils_tar_fd.write(response.content)

    print("Extracting GCC tarball...")
    with tarfile.open(tar_file, "r:xz") as binutils_tar_fd:
        binutils_tar_fd.extractall(path=".")

    os.remove(tar_file)

    os.chdir(tar_output_directory)
    sh.Command("./contrib/download_prerequisites")()
    os.chdir("..")

    os.makedirs(build_directory, exist_ok=True)
    os.chdir(build_directory)
    print("Configuring GCC...")
    sh.Command(f"{tar_output_directory}/configure")(
        "--host",
        get_system_triplet(),
        "--build",
        get_system_triplet(),
        "--prefix",
        toolchain_prefix,
        "--target",
        platform_prefix,
        "--disable-nls",
        "--enable-languages=c,c++",
        "--without-headers",
    )
    os.chdir("..")

    print("Building GCC...")
    make = sh.Command("make")
    make(
        "-C", build_directory, "all-gcc", "all-target-libgcc", "-j", str(os.cpu_count())
    )
    make("-C", build_directory, "install-gcc", "install-target-libgcc")


def main(
    platform_prefix: str,
    toolchain_directory: str,
    toolchain_prefix: str,
    binutils_url: str,
    binutils_version: str,
    gcc_url: str,
    gcc_version: str,
) -> None:
    print(
        f"Building toolchain for {platform_prefix} in {toolchain_directory} with prefix {toolchain_prefix}"
    )
    os.makedirs(toolchain_directory, exist_ok=True)

    chdir_wrapper(build_binutils, toolchain_directory)(
        platform_prefix.strip("-"), toolchain_prefix, binutils_url, binutils_version
    )
    chdir_wrapper(build_gcc, toolchain_directory)(
        platform_prefix.strip("-"), toolchain_prefix, gcc_url, gcc_version
    )


if __name__ == "__main__":
    if len(sys.argv) != 8:
        print(
            "Usage: python3 make-toolchain.py <platform prefix> <toolchain directory> <toolchain prefix> <binutils url> <binutils version> <gcc url> <gcc version>"
        )
        sys.exit(1)

    main(
        sys.argv[1],
        sys.argv[2],
        sys.argv[3],
        sys.argv[4],
        sys.argv[5],
        sys.argv[6],
        sys.argv[7],
    )
