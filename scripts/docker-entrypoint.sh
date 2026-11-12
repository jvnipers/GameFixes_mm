#!/bin/bash
set -e

git submodule update --init --recursive

# SEMVER comes in from the CI, whose version job resolves the tag -- on the
# pushbuild.txt path it is not on HEAD yet, so git describe cannot see it. A
# tag push and a local build still find it on HEAD.
if [ -n "${SEMVER:-}" ]; then
  export SEMVER
  echo "=== Version: $SEMVER (from the environment) ==="
elif SEMVER="$(git describe --tags --exact-match 2>/dev/null)"; then
  export SEMVER
  echo "=== Version: $SEMVER (tag on HEAD) ==="
else
  unset SEMVER
fi

export GITHUB_SHA_SHORT="$(git rev-parse --short HEAD)"

### --- Build ---------------------------------------------------------------
echo "=== Starting build ==="

rm -rf build
mkdir build
cd build

pwd
python ../configure.py \
  --enable-optimize \
  --sdks cs2
ambuild

echo "=== DONE ==="
