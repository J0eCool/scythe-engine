#!/usr/bin/sh

# build
./build-scythe.sh

# run
pushd out
./scythe
