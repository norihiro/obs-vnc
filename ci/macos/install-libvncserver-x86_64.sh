#! /bin/sh

set -ex

brew install pkg-config

brew install jpeg-turbo libpng lzo openssl libvncserver
cp /usr/local/opt/jpeg-turbo/LICENSE.md data/LICENSE-jpeg
cp /usr/local/opt/libpng/LICENSE data/LICENSE-libpng
cp /usr/local/opt/lzo/COPYING data/COPYING-lzo
cp /usr/local/opt/libvncserver/COPYING data/COPYING-libvncserver

sudo ed /usr/local/lib/pkgconfig/libvncclient.pc <<-EOF
/^Libs:/
.s;Libs: ;Libs: -L/usr/local/lib -L/usr/local/opt/jpeg-turbo/lib -L/usr/local/opt/openssl@1.1/lib ;
wq
EOF

#for debug
cat /usr/local/lib/pkgconfig/libvncclient.pc
