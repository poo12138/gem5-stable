#!/usr/bin/env bash
export RESULT_DIR=results/hotspot

rm -fr $RESULT_DIR
mkdir -p $RESULT_DIR

build/ARM/gem5.debug -d $RESULT_DIR configs/example/TD_v3_hotspot.py
