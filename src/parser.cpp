#include <iostream>

#include "parser.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYGLTF_NO_STB_IMAGE
#include "tinygltf/tiny_gltf.h"

std::vector<BasicModelData> ParseModel::parse_model(std::string path) 
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;
    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
    if (!warn.empty()) 
    {
        std::cout << "Warning: " << warn << std::endl;
    }

    if (!err.empty()) 
    {
        std::cout << "Error: " << err << std::endl;
    }

    if (!res)
    {
        std::cout << "Failed to load GLTF" << std::endl;
    } else 
    {
        std::cout << "Loaded 1 model successfully" << std::endl;
    }
}



int main () {
    ParseModel model_parser;
    model_parser.parse_model("../assets/Monster.gltf");
    return 0;
}
