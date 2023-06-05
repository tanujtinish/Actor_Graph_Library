#!/bin/bash

export LOCAL=/lscratch/forza/install
export BIN=$BIN:$LOCAL/bin
export CC=$LOCAL/bin/oshcc
export CXX=$LOCAL/bin/oshc++
export OSHRUN=$LOCAL/bin/oshrun
export BALE_INSTALL=/lscratch/forza/bale/src/bale_classic/build_unknown
export HCLIB_ROOT=/lscratch/forza/hclib/hclib-install
export LD_LIBRARY_PATH=$LOCAL/lib:$BALE_INSTALL/lib:$HCLIB_ROOT/lib:$HCLIB_ROOT/../modules/bale_actor/lib:$HCLIB_ROOT/../../ActorGraphBenchmark/src/bale3/bfs/src
export HCLIB_WORKERS=1


