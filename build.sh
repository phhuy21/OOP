#!/usr/bin/env bash
# Biên dịch SkyGate AMS bằng g++ (MSYS2 UCRT64).
# Cách dùng:  bash build.sh   ->  tạo ./skygate (hoặc skygate.exe trên Windows)
set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -Ithird_party"
# Console app dùng src/main.cpp. Loại web_main.cpp (định nghĩa main() riêng cho
# web server + cần winsock) để không trùng symbol main / thiếu thư viện socket.
# SQLite amalgamation được biên dịch riêng bằng C compiler.
SRC=$(find src -name '*.cpp' ! -name 'web_main.cpp')
OUT="skygate"

echo "Đang biên dịch ${OUT} ..."
# Biên dịch sqlite3.c thành object file C (KHÔNG dùng g++ cho file C).
gcc -c -O2 third_party/sqlite3.c -o /tmp/skygate_sqlite3.o
$CXX $CXXFLAGS $SRC /tmp/skygate_sqlite3.o -o "$OUT"
rm -f /tmp/skygate_sqlite3.o
echo "Xong. Chạy: ./$OUT"
