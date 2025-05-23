#!/usr/bin/python3

import sys
import sh


def main(image_type: str, image_path: str, memory_size: str) -> None:
    # run qemu
    sh.Command("qemu-system-i386")(
        "-m",
        memory_size,
        # "-hda",
        # image_path,
        "-drive",
        f"file={image_path},format=raw,if=ide",
        "-debugcon",  # for the e9 port hack
        "stdio",  #
        "-serial",  # disable com1 serial port (also for e9 port hack)
        "null",  #
        _out=sys.stdout,
        _err=sys.stderr,
    )


if __name__ == "__main__":
    if len(sys.argv) != 4:  # script name + 3 args
        print("Usage: python3 run.py <image type> <image path> <memory size>")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2], sys.argv[3])
