rm -rf /tmp/install_open-watcom-v2
mkdir /tmp/install_open-watcom-v2
cd /tmp/install_open-watcom-v2

if [ "$(uname -m)" = "x86_64" ]; then
    echo "Dowloading 64-bit version of Open Watcom v2..."
    wget "https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x64" -O open-watcom-v2-installer
else
    echo "Dowloading 32-bit version of Open Watcom v2..."
    wget "https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x86" -O open-watcom-v2-installer
fi

echo "In the installer you must select 16-bit compilers."
echo "And the installation directory must be /usr/bin/watcom"
read -p "Press enter to run the installer."

chmod +x open-watcom-v2-installer
sudo ./open-watcom-v2-installer