Azure SDK for C Contributing Guide
-------------------------------------

Thank you for your interest in contributing to Azure SDK for C.

- For reporting bugs, requesting features, or asking for support, please file an issue in the [issues](https://github.com/Azure/azure-sdk-for-c/issues) section of the project.

- If you would like to become an active contributor to this project please follow the instructions provided in [Microsoft Azure Projects Contribution Guidelines](http://azure.github.com/guidelines.html).

- To make code changes, or contribute something new, please follow the [GitHub Forks / Pull requests model](https://help.github.com/articles/fork-a-repo/): Fork the repo, make the change and propose it back by submitting a pull request.

- Refer to the [wiki](https://github.com/Azure/azure-sdk-for-c/wiki) to learn about how Azure SDK for C generates lint checker, doxygen, and code coverage reports.

Pull Requests
-------------

* **DO** submit all code changes via pull requests (PRs) rather than through a direct commit. PRs will be reviewed and potentially merged by the repo maintainers after a peer review that includes at least one maintainer.
* **DO NOT** submit "work in progress" PRs.  A PR should only be submitted when it is considered ready for review and subsequent merging by the contributor.
* **DO** give PRs short-but-descriptive names (e.g. "Improve code coverage for Azure.Core by 10%", not "Fix #1234")
* **DO** refer to any relevant issues, and include [keywords](https://help.github.com/articles/closing-issues-via-commit-messages/) that automatically close issues when the PR is merged.
* **DO** tag any users that should know about and/or review the change.
* **DO** ensure each commit successfully builds.  The entire PR must pass all tests in the Continuous Integration (CI) system before it'll be merged.
* **DO** address PR feedback in an additional commit(s) rather than amending the existing commits, and only rebase/squash them when necessary.  This makes it easier for reviewers to track changes.
* **DO** assume that ["Squash and Merge"](https://github.com/blog/2141-squash-your-commits) will be used to merge your commit unless you request otherwise in the PR.
* **DO NOT** fix merge conflicts using a merge commit. Prefer `git rebase`.
* **DO NOT** mix independent, unrelated changes in one PR. Separate real product/test code changes from larger code formatting/dead code removal changes. Separate unrelated fixes into separate PRs, especially if they are in different assemblies.

Merging Pull Requests (for project contributors with write access)
----------------------------------------------------------

* **DO** use ["Squash and Merge"](https://github.com/blog/2141-squash-your-commits) by default for individual contributions unless requested by the PR author.
  Do so, even if the PR contains only one commit. It creates a simpler history than "Create a Merge Commit".
  Reasons that PR authors may request "Merge and Commit" may include (but are not limited to):

  - The change is easier to understand as a series of focused commits. Each commit in the series must be buildable so as not to break `git bisect`.
  - Contributor is using an e-mail address other than the primary GitHub address and wants that preserved in the history. Contributor must be willing to squash
    the commits manually before acceptance.



## Developer Guide

### Pre-requisites

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

### Building and Testing

#### Building the project
First, ensure that the `VCPKG_ROOT` environment variable is set, as described [above](#vcpkg). This needs to be defined
any time you want to build. Then generate the build files and build as you would any standard CMake project. From the
repo root, run:

```sh
mkdir build
cd build
cmake -Duse_default_uuid=ON ..
cmake --build .
```

#### Testing the project
Tests are executed via the `ctest` command included with CMake. From the repo root, run:

```sh
cd build
ctest -C Debug
```

### Visual Studio 2019
You can also build the project by simply opening the desired project directory in Visual Studio. Everything should be
preconfigured to build and run tests.
