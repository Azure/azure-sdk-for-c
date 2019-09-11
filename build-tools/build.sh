#!/usr/bin/env bash

# Get path from where we are 
LOCATION="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/.."
BUILD_DIR="${LOCATION}/build"

# run cmake and build
cmake -s $LOCATION -B $BUILD_DIR
cmake --build $BUILD_DIR
