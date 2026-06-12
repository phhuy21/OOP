#!/usr/bin/env bash
# Biên dịch SkyGate AMS bằng g++ (MSYS2 UCRT64).
# Cách dùng:  bash build.sh   ->  tạo ./skygate (hoặc skygate.exe trên Windows)
set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2"
SRC=$(find src -name '*.cpp')
OUT="skygate"

echo "Đang biên dịch ${OUT} ..."
$CXX $CXXFLAGS $SRC -o "$OUT"
echo "Xong. Chạy: ./$OUT"
