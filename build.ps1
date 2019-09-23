$location = $PSScriptRoot
$build = Join-Path $location "build"
CMake -S $location -B $build
CMake --build $build --target RUN_TESTS
