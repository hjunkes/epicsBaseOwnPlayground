#!/bin/sh
set -e -x

die() {
  echo "$1" >&2
  exit 1
}

CACHEKEY=1

export EPICS_HOST_ARCH=`perl src/tools/EpicsHostArch.pl`

[ -e configure/os/CONFIG_SITE.Common.linux-x86 ] || die "Wrong location: $PWD"

case "$CMPLR" in
clang)
  echo "Host compiler is clang"
  cat << EOF >> configure/os/CONFIG_SITE.Common.$EPICS_HOST_ARCH
GNU         = NO
CMPLR_CLASS = clang
CC          = clang
CCC         = clang++
EOF
  ;;
*) echo "Host compiler is default";;
esac

if [ "$STATIC" = "YES" ]
then
  echo "Build static libraries/executables"
  cat << EOF >> configure/CONFIG_SITE
SHARED_LIBRARIES=NO
STATIC_BUILD=YES
EOF
fi

# requires wine and g++-mingw-w64-i686
if [ "$WINE" = "32" ]
then
  echo "Cross mingw32"
  sed -i -e '/CMPLR_PREFIX/d' configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
  cat << EOF >> configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
CMPLR_PREFIX=i686-w64-mingw32-
EOF
  cat << EOF >> configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS+=win32-x86-mingw
EOF
fi

# set RTEMS to eg. "4.9" or "4.10"
if [ -n "$RTEMS" ]
then
  echo "Cross RTEMS${RTEMS} for pc386"
  if [[ $RTEMS == 5* ]]; then 
    mkdir $HOME/RTEMS_DEV
    cd $HOME/RTEMS_DEV
    git clone git://git.rtems.org/rtems.git
    git clone git://git.rtems.org/rtems-tools.git
    git clone git://git.rtems.org/rtems-source-builder.git
    cd rtems-tools
    ./waf configure --prefix=$HOME/.rtems build install
    cd ../rtems-source-builder/rtems/
    ../source-builder/sb-set-builder --log log.i386.txt --prefix=$HOME/.rtems 5/rtems-i386
    cd $HOME/RTEMS_DEV/rtems
    ../rtems-source-builder/source-builder/sb-bootstrap
    cd ..
    mkdir -p build/b-i386
    ../../rtems/configure --enable-maintainer-mode --prefix=${HOME}/.rtems \
    --target=i386-rtems5 --enable-rtemsbsp="pc386" --enable-posix \
    --enable-cxx --enable-networking
    make all
    make install
  else 
    curl -L "https://github.com/mdavidsaver/rsb/releases/download/20171203-${RTEMS}/i386-rtems${RTEMS}-trusty-20171203-${RTEMS}.tar.bz2" \
    | tar -C / -xmj
  fi

  sed -i -e '/^RTEMS_VERSION/d' -e '/^RTEMS_SERIES/d' -e '/^RTEMS_BASE/d' configure/os/CONFIG_SITE.Common.RTEMS
  cat << EOF >> configure/os/CONFIG_SITE.Common.RTEMS
RTEMS_VERSION=$RTEMS
RTEMS_SERIES=$RTEMS
RTEMS_BASE=$HOME/.rtems
EOF

  cat << EOF >> configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS += RTEMS-pc386-qemu
EOF

  # find local qemu-system-i386
  echo -n "Using QEMU: "
  type qemu-system-i386 || echo "Missing qemu"
fi

make -j2 $EXTRA

if [ "$TEST" != "NO" ]
then
   make -j2 tapfiles
   make -s test-results
fi
