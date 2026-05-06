#!/bin/bash

make clean
make -j"$(nproc --all)"
sudo make install
