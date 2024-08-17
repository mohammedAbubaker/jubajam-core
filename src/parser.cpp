#include <cstring>
#include <iostream>
#include <unordered_map>
#include "parser.h"
#include "json.hpp"
#include "fstream"
#include "Base64/Base64.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

std::string encode(const std::string &data) {
    auto b64 = macaron::Base64::Encode(data);
    return b64;
}

int decode(const std::string &data, std::string &out) {
    auto error = macaron::Base64::Decode(data, out);
    if (!error.empty()) {
        std::cout << "Error: " << error << std::endl;
        return 1;
    }
    return 0;
}

std::vector<float> string_to_float(std::string buffer)
{
    std::vector<float> result;
    for (int i = 0; i < buffer.size(); i += 4) {
        unsigned char bytes[4] = {
            (unsigned char) buffer.at(i), 
            (unsigned char) buffer.at(i+1), 
            (unsigned char) buffer.at(i+2), 
            (unsigned char) buffer.at(i+3)
        };
        float float_value;
        std::memcpy(&float_value, bytes, sizeof(float));
        result.push_back(float_value);
    }
    return result;
}

std::string get_substring(std::string buffer, int bufferOffset, int bufferLength)
{
    return buffer.substr(bufferOffset, bufferOffset + bufferLength);
}

std::vector<RenderPart> ParseModel::parse_model(std::string path) 
{
    /*
     * The main goal at the moment is to have a function that returns
     * a list which contains a RenderPart. The RenderPart holds the necessary
     * texture data and vertex data to render that part. 
     *
     * A model would just be a vector of RenderParts.
    */

    std::vector<RenderPart> result;

    std::ifstream file_obj(path);
    nlohmann::json json_obj = nlohmann::json::parse(file_obj);

    // Process the buffers here
    std::unordered_map<std::string, std::string> buffers;
    int _i = 0;
    for (auto buffer: json_obj["buffers"]) {
        std::string uri = buffer["uri"];
        std::string data = uri.substr(uri.find(',') + 1, uri.size());
        std::string out;
        decode(data, out);
        buffers["Monster"] = out;
    }

    // Process mesh here
    for (auto mesh: json_obj["meshes"]) {
        for (auto primitive: mesh["primitives"]) {
            auto attributes = primitive["attributes"];
            std::string position_accessor_index = attributes["POSITION"];
            std::string texcoord_accessor_index = attributes["TEXCOORD_0"];

            auto position_accessor = json_obj["accessors"][position_accessor_index];
            auto texcoord_accessor = json_obj["accessors"][texcoord_accessor_index];
            
            std::string bv_position_index = position_accessor["bufferView"];
            std::string bv_texcoord_index = texcoord_accessor["bufferView"];

            auto bv_position = json_obj["bufferViews"][bv_position_index];
            auto bv_texcoord = json_obj["bufferViews"][bv_texcoord_index];

            std::vector<float> position_substring = string_to_float(
                    get_substring(
                        buffers["Monster"], 
                        bv_position["byteOffset"], 
                        bv_position["byteLength"]
                    )
            );

            std::vector<float> texcoord_substring = string_to_float(
                    get_substring(
                        buffers["Monster"], 
                        bv_texcoord["byteOffset"], 
                        bv_texcoord["byteLength"]
                    )
            );

            std::string uri = json_obj["images"]["monster_jpg"]["uri"];
            std::string data = uri.substr(uri.find(',') + 1, uri.size());
            std::string texture_data;
            decode(data, texture_data);
            
            unsigned char *image_data =  (unsigned char*) texture_data.c_str();
            int width, height, channels;
            unsigned char *image = stbi_load_from_memory(
                image_data,
                texture_data.length(),
                &width, 
                &height, 
                &channels, 
                0
            );

            std::vector<unsigned char> final_tex(image_data, image_data + (width * height * channels));
            TextureInfo texture_info = TextureInfo {
                .texture_data = final_tex,
                .width = width,
                .height = height,
                .channels = channels,
            };

            RenderPart resulting_renderpart = {
                .texture_info = texture_info,
                .position_data = position_substring,
                .texcoord_data = texcoord_substring
            };
            result.push_back(resulting_renderpart);
        }
    }
    return result;
}
