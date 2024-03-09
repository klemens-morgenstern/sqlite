#! /bin/sh

set -e

build_dir=$2

branch="master"

if [ "$1" != "master" -a "$1" != "refs/heads/master" ]; then
    branch="develop"
fi

echo "BUILD_DIR: $build_dir"
echo "BRANCH: $branch"

git clone -b $branch --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root

# Use a reasonably large depth to prevent intermittent update failures due to
# commits being on a submodule's master before the superproject is updated.
git submodule update --init --depth 20 --jobs 4 \
    libs/array \
    libs/headers \
    tools/build \
    tools/boost_install \
    tools/boostdep \
    libs/align \
    libs/asio \
    libs/assert \
    libs/config \
    libs/core \
    libs/callable_traits \
    libs/describe \
    libs/filesystem \
    libs/optional \
    libs/system \
    libs/move \
    libs/mp11 \
    libs/variant2 \
    libs/json

echo Submodule update complete

rm -rf libs/beast
cp -r $build_dir libs/beast
