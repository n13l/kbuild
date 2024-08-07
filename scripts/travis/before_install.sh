#!/bin/sh
set -e
export BUILD_MAJOR=0
export BUILD_MINOR=0
export BUILD_REVISION=1

num=`echo $TRAVIS_BRANCH | cut -d. -f1`
if [ $num = *[[:digit:]]* ]; then export BUILD_MAJOR=$num ; fi
num=`echo $TRAVIS_BRANCH | cut -d. -f2`
if [ $num = *[[:digit:]]* ]; then export BUILD_MINOR=$num ; fi
num=`echo $TRAVIS_BRANCH | cut -d. -f3`
if [ $num = *[[:digit:]]* ]; then export BUILD_REVISION=$num ; fi

export BUILD_OS_RELEASE=$(uname -r)
export BUILD_OS_NAME=$(uname -s)
export BUILD_OS_ARCH=$(uname -m)

export VERSION="$BUILD_MAJOR"
export PATCHLEVEL="$BUILD_MINOR"
export SUBLEVEL="$BUILD_REVISION"

shell_session_update() { :; }
wget http://ftp.gnu.org/pub/gnu/gperf/gperf-3.1.tar.gz
export NEWPWD=$PWD
mkdir $NEWPWD/local
tar -xvzf gperf-3.1.tar.gz -C $NEWPWD/local
echo "enter1: $NEWPWD/local/gperf-3.1"
sh -c 'cd $NEWPWD/local/gperf-3.1 && ./configure --prefix=$NEWPWD/local && make && make install'
#echo "enter2: $NEWPWD/local/gperf-3.1"
#./configure --prefix=$NEWPWD/local
#echo "enter3: $NEWPWD/local/gperf-3.1"
#make
#make install
#cd $NEWPWD

if [ "$BUILD_TARGET" == "win32" ]; then 
#  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys
#  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1397BC53640DB551
#  sudo apt-get update -q
# sudo add-apt-repository ppa:wine/wine-builds -y
#  sudo add-apt-repository ppa:ubuntu-wine/ppa -y
#  sudo apt-get update -q
#  sudo apt-get -f install
#  sudo apt-get install wine1.6 -y
# sudo apt-get install --install-recommends winehq-devel
  unset CC 
  export CROSS_COMPILE="i686-w64-mingw32-"
  export MINGW=/opt/mingw64 
  export PATH=$PATH:$MINGW/bin
  export BUILD_OS_NAME="win"
  export BUILD_OS_ARCH="x86_32"
  export BUILD_OS_RELEASE="generic"
  export WINEDEBUG=err-all,fixme-all
  export OS_EXEC="wine"
fi 
if [ "$BUILD_TARGET" == "win64" ]; then 
#  sudo dpkg --add-architecture i386
#  sudo apt-get -f install
#  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys
#  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1397BC53640DB551
#  sudo apt-get update -q
#
# sudo add-apt-repository ppa:wine/wine-builds -y
#  sudo add-apt-repository ppa:ubuntu-wine/ppa -y
#  sudo apt-get update -q
#  sudo apt-get -f install
#  sudo apt-get install wine1.6 -y
# sudo apt-get install --install-recommends winehq-devel
  unset CC 
  export CROSS_COMPILE="x86_64-w64-mingw32-"
  export MINGW=/opt/mingw64 
  export PATH=$PATH:$MINGW/bin
  export BUILD_OS_NAME="win"
  export BUILD_OS_ARCH="x86_64"
  export BUILD_OS_RELEASE="generic"
  export WINEDEBUG=err-all,fixme-all
  export OS_EXEC="wine"
fi
if [ "$BUILD_ARCH" == "s390x" ]; then
  echo "deb http://ftp.de.debian.org/debian sid main contrib non-free" | sudo tee -a /etc/apt/sources.list
  sudo apt-get update -qq
  sudo -E apt-get -yq --no-install-suggests --no-install-recommends --allow-unauthenticated install flex bison gperf pkg-config gcc-s390x-linux-gnu binutils-s390x-linux-gnu linux-libc-dev-s390x-cross libc6-s390x-cross libc6-dev-s390x-cross qemu-user-static binfmt-support
  unset CC
  export CROSS_COMPILE=s390x-linux-gnu-
  export OS_EXEC="echo"
  export BUILD_OS_ARCH="x86_64"
  export OS_EXEC=$(/bin/sh scripts/travis/s390x-ld.sh)
  echo "OS_EXEC=$OS_EXEC"
fi
if [ "$BUILD_ARCH" == "powerpc64" ]; then
  echo "deb http://ftp.de.debian.org/debian sid main contrib non-free" | sudo tee -a /etc/apt/sources.list
  sudo apt-get update -qq
  sudo -E apt-get -yq --no-install-suggests --no-install-recommends --allow-unauthenticated -o Dpkg::Options::="--force-overwrite" install flex bison gperf pkg-config gcc-multilib-powerpc64-linux-gnu binutils-powerpc64-linux-gnu qemu-user-static binfmt-support 
  unset CC
  export CROSS_COMPILE=powerpc64-linux-gnu-
  export OS_EXEC="echo"
  export BUILD_OS_ARCH="powerpc64"
  export OS_EXEC=$(/bin/sh scripts/travis/ppc64-ld.sh)
  echo "OS_EXEC=$OS_EXEC"
fi
if [ "$BUILD_ARCH" == "arm32" ]; then
  unset CC
  export CROSS_COMPILE=arm-linux-gnueabihf-
  export OS_EXEC="echo"
  export BUILD_OS_ARCH="arm32"
  # evil workarround sysroot
  export OS_EXEC=$(/bin/sh scripts/travis/arm-ld.sh)
  echo "OS_EXEC=$OS_EXEC"
fi
if [ "$BUILD_ARCH" == "arm64" ]; then
  unset CC
  export CROSS_COMPILE=aarch64-linux-gnu-
  export OS_EXEC="echo"
  export BUILD_OS_ARCH="arm64"
  # evil workarround sysroot
  export OS_EXEC=$(/bin/sh scripts/travis/arm64-ld.sh)
  echo "OS_EXEC=$OS_EXEC"
fi
if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  unset CROSS_COMPILE 
  export BUILD_OS_NAME="osx"
  brew update 
  brew install flex bison gperftools
fi

if [ "$BUILD_TARGET" == "linux" ]; then
  export BUILD_OS_NAME="linux"
fi

export VERSION="$BUILD_MAJOR"
export PATCHLEVEL="$BUILD_MINOR"
export SUBLEVEL="$BUILD_REVISION"

echo "build-id: $BUILD_ID"
echo "build-version: $VERSION"
echo "build-patchlevel: $PATCHLEVEL"
echo "build-sublevel: $SUBLEVEL"
echo "build-target: $BUILD_TARGET"
echo "build-branch: $TRAVIS_BRANCH"
echo "build-release: $BUILD_OS_RELEASE"
echo "build-name: $BUILD_OS_NAME"
