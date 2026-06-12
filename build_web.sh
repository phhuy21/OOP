#!/usr/bin/env bash
# Biên dịch SkyGate Web (REST API + giao diện) bằng g++ (MSYS2 UCRT64).
# Cách dùng:  bash build_web.sh   ->  tạo ./skygate_web (hoặc .exe trên Windows)
set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -Ithird_party"
# Toàn bộ logic OOP trong src/ TRỪ main.cpp console VÀ web_main.cpp (thêm tường minh bên dưới).
# Nếu không loại web_main.cpp ở đây, nó sẽ bị link 2 lần -> trùng symbol -> ld lỗi.
SRC=$(find src -name '*.cpp' ! -name 'main.cpp' ! -name 'web_main.cpp')
OUT="skygate_web"

# Windows cần ws2_32 cho socket (httplib).
# Static-link runtime để exe chạy được khi không có DLL mingw trên PATH.
LIBS=""
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) LIBS="-static -static-libgcc -static-libstdc++ -lws2_32" ;;
  *) LIBS="-lpthread" ;;
esac

echo "Đang biên dịch ${OUT} ..."
$CXX $CXXFLAGS $SRC src/web/web_main.cpp -o "$OUT" $LIBS
echo "Xong. Chạy: ./$OUT [port]   (mặc định 8080)"
