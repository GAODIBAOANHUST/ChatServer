#!/bin/bash

set -x

# 如果没有build目录,创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -r `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make