# Todo
- Implement double buffering for graphics.
- Make everything return an error value instead of printing the error. Thus moving the responibility of displaying errors to the caller.
    - Also standardize errors, so for example all disk read errors are the same value.
- Implement writing/creating directories and listing in FAT driver.
- Add LFS support in FAT driver.
- Add library, so we don't need to duplicate code with the wonky shared files system.
- Add timeouts in ATA driver.
- Implement DMA instead of polling in ATA driver.
