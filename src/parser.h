#pragma once
#include <vector>
#include <string>
// Responsible for packing
// all the info needed for a basic
// model render

struct TexObject
{
    std::vector<unsigned char> texture_data;
    int height;
    int width;
    int nrChannels;
};

struct BasicModelData  
{
    TexObject texture_object;
    std::vector<double> vertices;
};

struct ParseModel
{
    std::vector<BasicModelData> parse_model(std::string path);
};
