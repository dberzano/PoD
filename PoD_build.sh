#!/bin/bash

export PoDPrefix=$(cd `dirname "$0"`;pwd)

case "$1" in
  --deep) Deep=1 ;;
  --shallow) Deep=0 ;;
  *) echo 'Specify --deep or --shallow' ; exit 1 ;;
esac

if [ $Deep == 1 ] ; then

  cd "$PoDPrefix"/src || exit 1
  rm -rf build
  mkdir build
  cd build || exit 1

  cmake \
    -C ../PoD-*/BuildSetup.cmake \
    ../PoD-*/ \
    -DCMAKE_INSTALL_PREFIX="$PoDPrefix" \
    -DBoost_DEBUG="ON" || exit $?

  read -p 'Is this OK? Type "Yes" to build and install: ' Ans
  [ "$Ans" != 'Yes' ] && exit 1

fi

cd "$PoDPrefix"/src/build || exit 1
make -j $(( $(grep -c bogomips /proc/cpuinfo) + 3 )) install || exit $?

#source $PoDPrefix/PoD_env.sh || exit $?
#pod-server getbins || exit $?

# Create bins manually
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
tar czvvf pod-wrk-bin-3.14-Linux-amd64.tar.gz pod-wrk-bin || exit $?
touch pod-wrk-bin-3.14-Linux-x86.tar.gz
touch pod-wrk-bin-3.14-Darwin-universal.tar.gz
rm -rf pod-wrk-bin

echo All OK
