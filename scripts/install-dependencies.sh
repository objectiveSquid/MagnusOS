# python3.10 and pip setup
apt-get install software-properties-common -y
add-apt-repository ppa:deadsnakes/ppa -y
apt-get update -y
apt-get install python3.10 -y

apt-get install python3.10-distutils -y
curl -sS https://bootstrap.pypa.io/get-pip.py | python3.10

# other apt packages
apt-get install qemu-system-x86 gdb gcc make nasm mtools gcc-multilib -y

# pip packages
python3.10 -m pip install sh scons parted pillow bitarray
