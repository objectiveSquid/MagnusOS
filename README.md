# MagnusOS
This is a little hobby project of mine, maybe it will succeed, maybe not.<br>
I was following the [Making and OS](https://www.youtube.com/watch?v=MwPjvJ9ulSc&list=PLm3B56ql_akNcvH8vvJRYOc7TbYhRs19M) series by [Daedalus Community](https://www.youtube.com/@DaedalusCommunity) on Youtube, but at episode 5 (the one about reading disks), the tutorial simply didn't work. (I was probably doing it wrong...)<br>
I'm now starting over following [Building an OS](https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN) series by [nanobyte](https://www.youtube.com/@nanobyte-dev), also on Youtube.

## Requirements
### Building the OS
  - Open Watcom v2
  - make
  - nasm
  - wget (for downloading watcom)
```sh
sudo apt-get install make nasm wget
sudo ./scripts/install_open-watcom-v2.sh
```
```sh
make
```
### Debugging with Bochs
  - bochs
  - bochs-sdl
  - bochsbios
  - vgabios
```sh
sudo apt-get install bochs bochs-sdl bochsbios vgabios
```
```sh
./scripts/debug.sh
```
### Running with Qemu
  - qemu
```sh
sudo apt-get install qemu
```
```sh
./scripts/run.sh
```
### Building the tools
  - gcc
  - make
```sh
sudo apt-get install gcc make
```
```sh
make tools
```