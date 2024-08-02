rm -rf compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=on .
cd build
cmake ..
make
./jubajam-core
