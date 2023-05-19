#!/bin/bash

module load gcc
module load python/3.7
module unload mvapich2

INSTALL_DIR=$PWD/install

# libevent
if [ ! -d libevent-2.1.12-stable ]; then
    wget https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
    tar xvfz libevent-2.1.12-stable.tar.gz
    cd libevent-2.1.12-stable
    ./autogen.sh
    CC=gcc ./configure --prefix=$INSTALL_DIR
    make install
    cd ..
fi

# PMIx
if [ ! -d pmix-4.1.2 ]; then
    wget https://github.com/openpmix/openpmix/releases/download/v4.1.2/pmix-4.1.2.tar.gz
    tar xvfz pmix-4.1.2.tar.gz
    cd pmix-4.1.2
    ./autogen.pl
    CC=gcc ./configure --prefix=$INSTALL_DIR --with-libevent=$INSTALL_DIR
    make install
    cd ..
fi

if [ ! -d ucx-1.12.1 ]; then
#    wget https://github.com/openucx/ucx/releases/download/v1.13.0/ucx-1.13.0.tar.gz
    wget https://github.com/openucx/ucx/releases/download/v1.12.1/ucx-1.12.1.tar.gz
    tar xvfz ucx-1.12.1.tar.gz
    cd ucx-1.12.1
    CC=gcc CXX=g++ ./contrib/configure-release --prefix=$INSTALL_DIR
    make install
    cd ..
fi

# OpenMPI
OMPI_VERSION=4.1.4
OMPI_LOCAL=$PWD/ompi_install
if [ ! -d openmpi-${OMPI_VERSION} ]; then
    wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-${OMPI_VERSION}.tar.gz
    tar xvfz openmpi-${OMPI_VERSION}.tar.gz
    cd openmpi-${OMPI_VERSION}
    CC=gcc ./configure  --prefix=$OMPI_LOCAL --without-verbs --with-ucx=$INSTALL_DIR && make install
    cd ..
fi

if [ ! -d shcoll ]; then
    git clone https://github.com/tonycurtis/shcoll.git
    cd shcoll
    ./autogen.sh
    # USE SOS's header
    CC=gcc ./configure --prefix=$INSTALL_DIR --with-shmemh-dir=$OMPI_LOCAL/include
    make install
    cd ..
fi

if [ ! -d osss-ucx ]; then
    git clone https://github.com/openshmem-org/osss-ucx.git
    cd osss-ucx
    ./autogen.sh
    CC=gcc CXX=g++ ./configure --prefix=$INSTALL_DIR --with-pmix=$INSTALL_DIR --with-shcoll=$INSTALL_DIR --with-ucx=$INSTALL_DIR --disable-threads --with-heap-size=1G
    make install
    cd ..
fi

# bale
if [ ! -d bale ]; then
    git clone https://github.com/jdevinney/bale.git
    cd bale/src/bale_classic/
    ./bootstrap.sh
    CC=$INSTALL_DIR/bin/oshcc ./make_bale -s -f
fi

export PATH=$INSTALL_DIR/bin:$PATH:$OMPI_LOCAL/bin

