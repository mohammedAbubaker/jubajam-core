#include <iostream>

#include "parser.h"
#include <fstream>
#include <sstream>

std::string read_model(std::string file_path)
{
    std::ifstream file(file_path.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void ParseOBJ::print_obj() {
    std::cout << "bazooka" << std::endl;
};

void GLTFfile::lol() {
    std::cout << "looool" << std::endl;
};

int main () {
    std::string model = read_model("../assets/Monster.gltf");
    std::cout << model << std::endl;
}
