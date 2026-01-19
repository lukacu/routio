#!/bin/bash

# Build for raspberry pi (aarch64, armv7l) and amd64 (x86_64) on a x86_64 host using cibuildwheel
# Requires Docker and cibuildwheel (pip install cibuildwheel)
#
# Usage: ./build_wheels.sh [--force]
#   --force: Rebuild all wheels even if they already exist

set -e

# Parse command-line arguments
FORCE_REBUILD=false
for arg in "$@"; do
    case $arg in
        --force)
            FORCE_REBUILD=true
            shift
            ;;
        *)
            ;;
    esac
done

# Check if Docker is installed
if ! command -v docker &> /dev/null
then
    echo "Docker could not be found. Please install Docker to proceed."
    exit 1
fi

# Check if cibuildwheel is installed
if ! python3 -m pip show cibuildwheel &> /dev/null
then
    echo "cibuildwheel is not installed. Installing cibuildwheel..."
    exit 1
fi

# Handle binfmt setup for cross-compilation
docker run --privileged --rm tonistiigi/binfmt --install all

# Target architectures
export CIBW_ARCHS_LINUX="aarch64 x86_64 armv7l"

# Use manylinux images (broadest compatibility for Debian-based RPi OS)
export CIBW_MANYLINUX_AARCH64_IMAGE="quay.io/pypa/manylinux2014_aarch64"
export CIBW_MANYLINUX_ARMV7L_IMAGE="quay.io/pypa/manylinux_2_17_armv7l"

# If targeting Alpine-based systems, build musllinux wheels too:
# export CIBW_MUSLLINUX_AARCH64_IMAGE="quay.io/pypa/musllinux_1_2_aarch64"

# (Optional) pass env to build system
export CIBW_ENVIRONMENT='CFLAGS="-O3"'
export CIBW_BUILD="cp310-* cp311-* cp312-* cp313-*"

ROOT=$(dirname $(dirname $(realpath $0)))

mkdir -p $ROOT/build

pushd $ROOT/build
cmake .. && make -j4
popd

OUTPUT="$ROOT/build/wheelhouse"

mkdir -p $OUTPUT

# Determine which builds to skip based on existing wheels
SKIP_BUILDS=""
if [ "$FORCE_REBUILD" = false ] && [ -d "$OUTPUT" ]; then
    # Check for existing wheels and build skip list
    for wheel in "$OUTPUT"/*.whl; do
        if [ -f "$wheel" ]; then
            # Extract version info from wheel filename 
            basename=$(basename "$wheel" .whl)
            # Skip this build - example: cp310-linux_aarch64
            if [[ $basename =~ (cp[0-9]+).*linux_([^-]+)$ ]]; then
                python_ver="${BASH_REMATCH[1]}"
                arch="${BASH_REMATCH[2]}"
                if [ -z "$SKIP_BUILDS" ]; then
                    SKIP_BUILDS="${python_ver}-linux_${arch}"
                else
                    SKIP_BUILDS="${SKIP_BUILDS},${python_ver}-linux_${arch}"
                fi
            fi
        fi
    done
fi

# Apply skip if we found existing wheels
if [ -n "$SKIP_BUILDS" ]; then
    echo "Skipping already-built wheels: $SKIP_BUILDS"
    export CIBW_SKIP="$SKIP_BUILDS"
else
    if [ "$FORCE_REBUILD" = true ]; then
        echo "Force rebuild enabled - rebuilding all wheels"
    fi
fi
echo "$ROOT/build/python"
# Kick off the build
pushd $ROOT/build
python3 -m cibuildwheel --platform linux --debug-traceback --output-dir $OUTPUT $ROOT/build/python
popd 
