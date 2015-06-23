Building on Linux is very easy. Just make sure you have the following packages installed:

    gcc
    make
    cmake
    boost 1.46 (or later)

For example, on Ubuntu, you can run:

    sudo apt-get install gcc cmake make libboost-all-dev

Run the build_linux.sh script to compile SkypeExport:

    ./build_linux.sh

The SkypeExport binary is created in the release folder, and will be either 32 or 64-bit depending on your current platform.

