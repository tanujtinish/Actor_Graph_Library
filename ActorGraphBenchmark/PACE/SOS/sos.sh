#!/bin/bash

module load gcc python/3.7.4.lua

INSTALL_DIR=$PWD/install

# OFI
if [ ! -d libfabric ]; then
    git clone https://github.com/ofiwg/libfabric
    cd libfabric
    git checkout tags/v1.10.0rc3 -b v1.10.0rc3
    ./autogen.sh && ./configure --prefix=$INSTALL_DIR && make install
    cd ..
fi

# HYDRA (mpirun)
if [ ! -d hydra-3.3.2 ]; then
    wget http://www.mpich.org/static/downloads/3.3.2/hydra-3.3.2.tar.gz
    tar xvfz hydra-3.3.2.tar.gz
    cd hydra-3.3.2
    ./configure --prefix=$INSTALL_DIR &&  make install
    cd ..
fi

# SOS
if [ ! -d SOS ]; then
    git clone https://github.com/Sandia-OpenSHMEM/SOS.git
    cd SOS
    git checkout tags/v1.4.5 -b v1.4.5
    ./autogen.sh && ./configure --prefix=$INSTALL_DIR --with-ofi=$INSTALL_DIR --disable-fortran --enable-pmi-simple --disable-error-checking --enable-memcpy && make install
    cd ..
fi

# bale
if [ ! -d bale ]; then
    git clone https://github.com/jdevinney/bale.git
    cd bale/src/bale_classic/
    ./bootstrap.sh
    CC=$INSTALL_DIR/bin/oshcc PLATFORM=sos ./make_bale -s -f
fi

export PATH=$INSTALL_DIR/bin:$PATH
