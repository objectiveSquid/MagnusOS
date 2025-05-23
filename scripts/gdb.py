#!/usr/bin/python3

import tempfile
import shutil
import sys
import os


def make_gdbscript(image_type: str, image_path: str, memory_size: str) -> str:
    path = tempfile.mkdtemp(prefix="magnusos") + "/gdbscript.gdb"

    with open(path, "w") as fd:
        fd.write("set disassembly-flavor intel\n")
        fd.write("b *0x7C00\n")
        fd.write("layout asm\n")
        fd.write(
            f"target remote | qemu-system-i386 -S -gdb stdio -m {memory_size} -drive file={image_path},format=raw,if=ide"
        )

    return path


def main(image_type: str, image_path: str, memory_size: str) -> None:
    # make script
    gdbscript_path = make_gdbscript(image_type, image_path, memory_size)

    # run gdb (use this because i need stdio directly)
    os.system(" ".join(["gdb", "-x", gdbscript_path]))

    # cleanup
    shutil.rmtree(os.path.dirname(gdbscript_path))


if __name__ == "__main__":
    if len(sys.argv) != 4:  # script name + 3 args
        print("Usage: python3 gdb.py <image type> <image path> <memory size>")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2], sys.argv[3])
