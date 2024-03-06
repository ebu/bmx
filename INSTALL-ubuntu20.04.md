# Install on Ubuntu20.04

## Requirements

Bare bones build of Ubuntu20.04 LTS Server

## Installation
### build system
sudo apt update
sudo apt install build-essential cmake pkg-config

### dependencies
sudo apt install uuid-dev libexpat-dev liburiparser-dev

### get source
git clone git@github.com:MarkusPfundstein/dolby-muxer.git
cd dolby-muxer

### init submodules with latest branch
git submodule update --init
cd deps/libMXF
git pull origin sadm
cd ..
cd deps/libMXF++
git pull origin sadm
cd ../../

### build
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
cmake --build .

### install
sudo make install
sudo ldconfig


