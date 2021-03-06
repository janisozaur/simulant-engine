
# Fedora (Linux) Installation for Contributors

*This process is for compiling Simulant from source, if you just want to get started using Simulant to build a game you probably want the [normal installation](install_fedora.md).*

The following installation process is probably very similar for other Linux distributions (such as Ubuntu, Debian etc.) the main difference should
only be the packages which must be installed.

# Prerequisites

First you must install the required packages using the package manager (in recent versions of Fedora this is the `dnf` command). Run the following command
in the terminal to install the packages:


```
sudo dnf install SDL2-devel openal-soft-devel mesa-libGL-devel git python
```

# Cloning and Compiling

Navigate in the terminal to where you want to clone Simulant, then run the following commands:

```
git clone --recursive https://github.com/Kazade/simulant-engine.git
mkdir simulant-engine/build
cd simulant-engine/build
cmake ..
make
sudo make install
```

You will be prompted for your password at the final command as that will install Simulant to your `/usr/local` directory which is not owned by you
and so requires root access.

# Running the Tests

If you navigate to the build directory you can run the tests with the following command

```
./tests/simulant_tests
```

# Cross-compiling for the Dreamcast


To cross-compile Simulant for the Dreamcast, you'll need to install Docker and to run the sample apps, you'll need an emulator like lxdream:

```
sudo dnf install docker lxdream
```

Then, you'll need to start the Docker service and pull the latest dreamcast-sdk image

```
sudo service docker start
sudo docker pull kazade/dreamcast-sdk
```

Finally there is a bash script which generates a build in the "dbuild" directory:

```
./dc-build.sh
```

This will build the DC port, and compile the samples to .cdi CD images. You can run them using lxdream with the following command (for example):

```
lxdream -b dbuild/samples/light_sample.cdi
```

# Cross-compiling for Dreamcast without Docker

To compile for the Dreamcast you need to make the following changes to the C++ flags in your
compiled KallistiOS SDK:

 - Enable RTTI
 - Enable Exceptions
 - Enable Operator names
 
e.g.

```
sed -i 's/-fno-rtti//' /opt/toolchains/dc/kos/environ_base.sh
sed -i 's/-fno-exceptions//' /opt/toolchains/dc/kos/environ_base.sh
sed -i 's/-fno-operator-names//' /opt/toolchains/dc/kos/environ_base.sh
```

# Cross-compiling for Windows (Not yet compiling)

To cross compile for Windows, you'll find need to install some additional dependencies:

```
sudo dnf install mingw64-SDL2 mingw64-openal-soft mingw64-zlib
```

Then, create a new folder to build into, and run `mingw64-cmake` instead of `cmake`:

```
mkdir buildw
cd buildw
mingw64-cmake ..
make
```

# Using the Compiled Simulant

To avoid confusion, uninstall simulant-tools from pip if you have it installed (e.g. pip uninstall simulant-tools).

Make sure that `which simulant` returns `/usr/local/bin/simulant`.

From then you can use the `simulant` command as normal, except pass `--use-global-simulant` to use the version you compiled and installed.
