#!/bin/bash
set -e

[ ! -d build ] || rm -rf build

cmake -Bbuild
cd build
make && make install
cd ../env
/bin/bash launch.sh