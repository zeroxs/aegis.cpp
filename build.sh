#!/bin/sh

(cd ./build && rm -rf * && cmake -DBUILD_EXAMPLES=1 .. && make -j16)