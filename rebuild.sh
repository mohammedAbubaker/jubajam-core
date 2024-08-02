export CXX="/usr/bin/clang++"
rm -rf build
cmake . -B build/
cd build
make
./jubajam-core
