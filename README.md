# MagnusOS
I was following the [Making an OS](https://www.youtube.com/watch?v=MwPjvJ9ulSc&list=PLm3B56ql_akNcvH8vvJRYOc7TbYhRs19M) series by [Daedalus Community](https://www.youtube.com/@DaedalusCommunity) on Youtube, but at episode 5 (the one about reading disks), the tutorial simply didn't work. (I was probably doing it wrong...)<br>
I'm now starting over following [Building an OS](https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN) series by [nanobyte](https://www.youtube.com/@nanobyte-dev), also on Youtube.

## Requirements
### WARNING: These instructions are only for debian-based linux distributions, for anything else, you are on your own!
### Building the OS
For building the OS you will need to set up a crosscompiler, this is done in `build-scripts/toolchain.mk`
  - make
  - nasm
  - wget (for downloading files)
  - mtools
```sh
sudo apt-get update
sudo apt-get install make nasm mtools
make build_toolchain
```
```sh
make
```
### Debugging with Bochs (Not currently working!)
  - bochs
  - bochs-x
  - bochsbios
  - vgabios
```sh
sudo apt-get update
sudo apt-get install bochs bochs-sdl bochsbios vgabios
```
```sh
./scripts/debug.sh
```
### Running with Qemu
  - qemu-system-x86
```sh
sudo apt-get update
sudo apt-get install qemu
```
```sh
./scripts/run_gui.sh
```
### Building the tools
  - gcc
  - make
```sh
sudo apt-get update
sudo apt-get install gcc make
```
```sh
make tools
```
