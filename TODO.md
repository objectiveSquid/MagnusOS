# Todo
- Implement writing/creating directories in FAT driver.
- Add LFS support in FAT driver.
- Implement DMA instead of polling in ATA driver.
- Make bootloader stage 2 an elf.
- Make SCons not pollute the src directory with object files.
- Add more function lookups in ELF.
- A shell? Compile with: sudo .toolchain/i686-elf/bin/i686-elf-gcc shell.c -o shell.elf -ffreestanding -fno-stack-protector -fPIC -pie -nostdlib -nostartfiles -Wl,-e,_start -Wl,--unresolved-symbols=ignore-all
