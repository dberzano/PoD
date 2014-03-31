#!/bin/bash

#
# PoD_build.sh -- by Dario Berzano <dario.berzano@cern.ch>
#
# Builds and installs PoD for SLC6. Directory structure expected:
#
# $PoDPrefix/
#  - src/
#     - build/                        # created by installer
#     - PoD-<version>-Source.tar.gz   # you must download it
#     - PoD-<version>-Source          # you must unpack it
#
# The source tarfile must be downloaded, unpacked and (maybe) patched manually.
#
# To install:
#
# $> cd <root_of_PoD_source>
# $> ./PoD_build.sh --deep   # will run CMake + make install
#
# To recompile after edits:
#
# $> ./PoD_build.sh --shallow   # will make install only
#

# Check if we are running from the root of the source code
cd $( dirname "$0" )
if [ ! -e "BuildSetup.cmake" ] ; then
  echo "The PoD build script is in the wrong directory"
  exit 1
fi

# The PoD version
if [[ $(basename "$PWD") =~ PoD-(.*)-Source ]] ; then
  export PoDVersion="${BASH_REMATCH[1]}"
  echo -e "\033[35mDetected PoD version: $PoDVersion\033[m"
else
  echo "Cannot detect PoD version from source directory name"
  exit 1
fi

# The installation prefix
export PoDPrefix=$(cd ../..;pwd)

# Choose between deep and shallow installation: the first runs everything
# including CMake after cleaning up the build directory, while the latter runs
# only make install
case "$1" in
  --deep) Deep=1 ;;
  --shallow) Deep=0 ;;
  *) echo 'Specify --deep or --shallow' ; exit 1 ;;
esac

if [ $Deep == 1 ] ; then

  # Clean up build and run CMake: requires interactive confirmation

  cd "$PoDPrefix"/src || exit 1
  rm -rf build
  mkdir build
  cd build || exit 1

  cmake \
    -C ../PoD-*/BuildSetup.cmake \
    ../PoD-*/ \
    -DCMAKE_INSTALL_PREFIX="$PoDPrefix" \
    -DBoost_DEBUG="ON" || exit $?

  echo -n -e "\033[35mIs this OK? Type \"Yes\" to build and install: \033[m"
  read Ans
  [ "$Ans" != 'Yes' ] && exit 1

fi

# Runs make install
cd "$PoDPrefix"/src/build || exit 1
make -j $(( $(grep -c bogomips /proc/cpuinfo) + 3 )) install || exit $?

# Create worker node bins manually: assumes we are on SLC6 with libboost
# installed "somewhere" and "accessible"
cd "$PoDPrefix"/bin || exit 1
rm -rf wn_bins
mkdir -p wn_bins/pod-wrk-bin
cd wn_bins/pod-wrk-bin
cp -v \
  "$PoDPrefix"/bin/private/ssh-tunnel \
  "$PoDPrefix"/bin/pod-agent \
  "$PoDPrefix"/bin/pod-user-defaults \
  "$PoDPrefix"/lib/libpod_protocol.so \
  "$PoDPrefix"/lib/libproof_status_file.so \
  "$PoDPrefix"/lib/libSSHTunnel.so \
  .
cd ..
tar czvvf "pod-wrk-bin-${PoDVersion}-Linux-amd64.tar.gz" pod-wrk-bin || exit $?
touch "pod-wrk-bin-${PoDVersion}-Linux-x86.tar.gz"
touch "pod-wrk-bin-${PoDVersion}-Darwin-universal.tar.gz"
rm -rf pod-wrk-bin

echo -e "\033[32mAll OK\033[m"
