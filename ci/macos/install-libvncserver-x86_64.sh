#! /bin/bash

set -ex

brew install pkg-config

brew install libvncserver
cp /usr/local/opt/libvncserver/COPYING data/COPYING-libvncserver
