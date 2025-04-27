set disassembly-flavor intel
layout asm
target remote | qemu-system-i386 -S -gdb stdio -m 32 -hda build/main_disk.raw