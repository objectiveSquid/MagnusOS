#!/usr/bin/python3

import sys
import sh


def main(image_path: str, memory_size: str) -> None:
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
        "-no-reboot",  # do not reboot on triple fault
        _out=sys.stdout,
        _err=sys.stderr,
    )


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 run.py <image path> <memory size>")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])
