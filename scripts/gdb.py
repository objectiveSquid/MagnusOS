#!/usr/bin/python3

import tempfile
import shutil
import sys
import os


def make_gdbscript(kernel_path: str, image_path: str, memory_size: str) -> str:
    path = tempfile.mkdtemp(prefix="magnusos") + "/gdbscript.gdb"

    with open(path, "w") as fd:
        fd.write(
            f"""set disassembly-flavor intel
set architecture i386
layout src
symbol-file {kernel_path}

target remote | qemu-system-i386 -S -gdb stdio \
-m {memory_size} \
-drive file={image_path},format=raw,if=ide \
-debugcon file:E9.log \
-serial null \
-no-reboot
"""
        )

    return path


def main(kernel_path: str, image_path: str, memory_size: str) -> None:
    # make script
    gdbscript_path = make_gdbscript(kernel_path, image_path, memory_size)

    # run gdb (use this because i need stdio directly)
    os.system(" ".join(["gdb", "-tui", "-x", gdbscript_path]))

    # cleanup
    shutil.rmtree(os.path.dirname(gdbscript_path))


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python3 gdb.py <kernel path> <image path> <memory size>")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2], sys.argv[3])
