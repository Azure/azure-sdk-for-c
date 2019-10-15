# How to contribute

If you encounter any bugs with the library please file an issue in the
[Issues](https://github.com/Azure/azure-sdk-for-c/issues) section of the project.

If you would like to become an active contributor to this project please follow the instructions provided in [Microsoft
Azure Projects Contribution Guidelines](http://azure.github.com/guidelines.html).

## Prerequisites
This project contains Git submodules which are required to build. After cloning this repo, run `git submodule update
--init --recursive`.

### CMake
CMake version 3.12 or higher is required to build these libraries. Download and install CMake from the project's
[website](https://cmake.org/download/).

### Vcpkg
Vcpkg is required to download project dependencies. To get started, first clone vcpkg to a location on your system and
run the bootstrapping script.

```sh
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg # Keep note of the location of this directory for the next step
Windows> .\bootstrap-vcpkg.bat
Linux/macOS:~/$ ./bootstrap-vcpkg.sh
```

On macOS, this command may fail if your version of the C++ toolchain is not new enough to support vcpkg. To resolve
this, vcpkg recommends to install `gcc@6` from Homebrew (`brew install gcc@6`), then re-run the bootstrapping script.

Next, define the `VCPKG_ROOT` environment variable and add the `vcpkg` command to your path. You will probably want to
persist these changes, so it's recommended to add/edit them via the Windows "System Properties" control panel, or via
your `.profile` file on Linux/macOS.

> **Windows**
> ```bat
> set VCPKG_ROOT=C:\path\to\vcpkg
> set PATH=%PATH%;%VCPKG_ROOT%
> ```
>
> **Linux/macOS**
> ```sh
> export VCPKG_ROOT=/path/to/vcpkg
> export PATH=$PATH:$VCPKG_ROOT
> ```

Finally, install the project dependencies with vcpkg.

> **Windows**
> ```bat
> set VCPKG_DEFAULT_TRIPLET=x64-windows-static
> vcpkg install curl[winssl]
> ```
>
> **Linux/macOS**
> ```sh
> vcpkg install curl[ssl]
> ```

### Development headers (Linux/macOS)
On Linux/macOS the development headers for OpenSSL 1.1 must be installed to a location where CMake can find them.
For Ubuntu 18.04 and up, you can install them directly from the main Ubuntu repository with `apt-get`. For macOS, you
can install them with Homebrew.

> **Linux (Ubuntu 18.04 and up)**
> ```sh
> sudo apt-get install libssl-dev
> ```
>
> **macOS**
> ```sh
> brew install openssl@1.1
> ```

## Building the project
First, ensure that the `VCPKG_ROOT` environment variable is set, as described [above](#vcpkg). This needs to be defined
any time you want to build. Then generate the build files and build as you would any standard CMake project. From the
repo root, run:

```sh
mkdir build
cd build
cmake -Duse_default_uuid=ON ..
cmake --build .
```

## Testing the project
Tests are executed via the `ctest` command included with CMake. From the repo root, run:

```sh
cd build
ctest -C Debug
```

### Visual Studio 2019
You can also build the project by simply opening the desired project directory in Visual Studio. Everything should be
preconfigured to build and run tests.
