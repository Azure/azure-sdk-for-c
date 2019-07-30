
## Requirements

This azure sdk module has several external build-time requirements.

- **Catch2** (for tests): https://github.com/catchorg/Catch2.git
- **extra-cmake-modules**: https://github.com/KDE/extra-cmake-modules.git

these may be installed using a package manager or embedded in the external/ folder.

By default dependencies will be found via the system, to enable embedding set
`-Daz_exampleshortname_allow_embed=ON`.