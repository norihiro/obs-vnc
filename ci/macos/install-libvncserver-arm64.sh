#! /bin/bash

set -ex

brew install pkg-config

curl -L -o deps.tar.gz http://www.nagater.net/obs-studio/obs-vnc-brew-apple-deps-20240101.tar.gz
sha256sum -c <<-EOF
f70b769c8e89a726d82c7f45ee4517ab209201ba57aa2700478309e4d2ac9f2c  deps.tar.gz
EOF
(cd / && sudo tar xz) < deps.tar.gz
