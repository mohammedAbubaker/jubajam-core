export CXX="/usr/bin/clang++"

rm -rf compile_commands.json
rm -rf build-cmake

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=on .

cmake -Bbuild-cmake -H. -GNinja
cd build-cmake && cmake --build .
./jubajam-core
