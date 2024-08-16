#pragma once
#include <vector>
#include <string>
// Responsible for packing
// all the info needed for a basic
// model render

struct RenderPart 
{
    std::string textures;
    std::vector<float> position_data;
    std::vector<float> texcoord_data;
};

struct ParseModel
{
    std::vector<RenderPart> parse_model(std::string path);
};
