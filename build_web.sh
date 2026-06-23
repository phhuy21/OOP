#!/usr/bin/env bash
# Biên dịch SkyGate Web (REST API + giao diện) bằng g++ (MSYS2 UCRT64).
# Cách dùng:  bash build_web.sh   ->  tạo ./skygate_web (hoặc .exe trên Windows)
set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -Ithird_party"
# Toàn bộ logic OOP trong src/ TRỪ main.cpp console VÀ web_main.cpp (thêm tường minh bên dưới).
# Nếu không loại web_main.cpp ở đây, nó sẽ bị link 2 lần -> trùng symbol -> ld lỗi.
# SQLite amalgamation được biên dịch riêng bằng C compiler.
SRC=$(find src -name '*.cpp' ! -name 'main.cpp' ! -name 'web_main.cpp')
OUT="skygate_web"

# Windows cần ws2_32 cho socket (httplib).
# Static-link runtime để exe chạy được khi không có DLL mingw trên PATH.
LIBS=""
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) LIBS="-static -static-libgcc -static-libstdc++ -lws2_32" ;;
  *) LIBS="-lpthread -ldl" ;;
esac

echo "Đang biên dịch ${OUT} ..."
# Biên dịch sqlite3.c thành object file C (KHÔNG dùng g++ cho file C).
gcc -c -O2 third_party/sqlite3.c -o /tmp/skygate_sqlite3.o
$CXX $CXXFLAGS $SRC src/web/web_main.cpp /tmp/skygate_sqlite3.o -o "$OUT" $LIBS
rm -f /tmp/skygate_sqlite3.o
echo "Xong. Chạy: ./$OUT [port]   (mặc định 8080)"
