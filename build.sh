#!/usr/bin/env bash
# Biên dịch SkyGate AMS bằng g++ (MSYS2 UCRT64).
# Cách dùng:  bash build.sh   ->  tạo ./skygate (hoặc skygate.exe trên Windows)
set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2"
# Console app dùng src/main.cpp. Loại web_main.cpp (định nghĩa main() riêng cho
# web server + cần winsock) để không trùng symbol main / thiếu thư viện socket.
SRC=$(find src -name '*.cpp' ! -name 'web_main.cpp')
OUT="skygate"

echo "Đang biên dịch ${OUT} ..."
$CXX $CXXFLAGS $SRC -o "$OUT"
echo "Xong. Chạy: ./$OUT"
