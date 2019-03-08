#! /bin/bash

# ROOOT!
if [ "$EUID" -eq 0 ]
  then echo "Please do not run as root"
  exit
fi

# User Warning
read -p "This script is a one-trick pony for ubuntu on windows and may mess up your system. Do you want to continue?" -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    [[ "$0" = "$BASH_SOURCE" ]] && exit 1 || return 1 # handle exits from shell or function but don't exit interactive shell
fi

# Install stuff
sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-pip python-setuptools python-serial python-cryptography python-future python-pyparsing grabserial

# download and extract toolchain
cd ~
mkdir -p ~/esp
cd ~/esp
wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
tar -xzf xtensa-esp32*.tar.gz

# download and extract idf
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git

# add path to profile
echo 'export PATH="$HOME/esp/xtensa-esp32-elf/bin:$PATH"' >> ~/.profile
echo 'export IDF_PATH="$HOME/esp/esp-idf"' >> ~/.profile

export PATH="$HOME/esp/xtensa-esp32-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/esp-idf"

python -m pip install --user -r $IDF_PATH/requirements.txt

# add user to dialout group
sudo usermod -a -G dialout $USER

echo "done."
echo 
echo "If you use windows use chmod 666 /dev/ttyS<No of COM Port>"