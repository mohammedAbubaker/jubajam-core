#include <iostream>

#include "parser.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

std::string read_model(std::string file_path)
{
    std::ifstream file(file_path.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<BasicModelData> ParseModel::parse_model(std::string path) 
{
    nlohmann::json model = nlohmann::json::parse(read_model(path));
    for (auto it : model.items()) 
    {
        std::cout << "type: " << it.value().type_name() << std::endl;
    }
}



int main () {
    ParseModel model_parser;
    model_parser.parse_model("../assets/Monster.gltf");
    return 0;
}
