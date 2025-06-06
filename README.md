# MagnusOS
I was following the [Making an OS](https://www.youtube.com/watch?v=MwPjvJ9ulSc&list=PLm3B56ql_akNcvH8vvJRYOc7TbYhRs19M) series by [Daedalus Community](https://www.youtube.com/@DaedalusCommunity) on Youtube, but at episode 5 (the one about reading disks), the tutorial simply didn't work. (I was probably doing it wrong...)<br>
I'm now starting over following [Building an OS](https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN) series by [nanobyte](https://www.youtube.com/@nanobyte-dev), also on Youtube.

## Download
You will need git, you can download it [here](https://git-scm.com/) or install it with your package manager (apt, dnf, yum, etc...).
```sh
git clone https://github.com/MagnusOS/MagnusOS.git
cd MagnusOS
```
And since it contains submodules, you will need to initialize them as such:
```sh
git submodule update --init --recursive
```

## Requirements
### WARNING: These instructions are only for debian-based linux distributions, for anything else, you are on your own!
### Building the OS
For building the OS you will need to set up a crosscompiler, this is done in `build_scripts/toolchain.py`.
To build anything, you will need to install the dependencies as such:
```sh
sudo ./scripts/install-dependencies.sh
scons toolchain
```
Then you can build the OS, *yes you will sadly need `sudo`, it's for mounting the filesystem to copy files to the image*:
```sh
sudo scons
```
### Running with QEMU
```sh
scons run
```
### Debugging with GDB + QEMU
```sh
scons gdb
```
