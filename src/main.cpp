#include <format>
#include <unordered_map>
#include "glm/ext/matrix_float3x3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <cfloat> 
#include <algorithm> 
#include <fstream>
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <GLFW/glfw3.h>
#include <assimp/material.h>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void print_vector(const glm::vec3 &vec) {
    std::cout << "{x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << "}" << std::endl;
}


// Helper function to convert aiMatrix4x4 to glm::mat4
glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}


struct vertex {
    glm::vec3       position;
    glm::vec3       normal;
    glm::vec2       tex_coords;
};

struct texture {
    unsigned int id;
    std::string type;
    std::string path;
    unsigned int texture_unit;
};

struct mesh {
    std::vector<vertex>         vertices;
    std::vector<unsigned int>   indices;
    std::unordered_map<std::string, texture>        textures;
    unsigned int VAO, VBO, EBO;
};

struct model {
    std::vector<mesh>   representation;
    glm::vec3           position;
    glm::vec3           bb_upper;
    glm::vec3           bb_lower;
    glm::vec3           bb_centre;
    GLuint              bbox_vao;  
    GLuint              bbox_vbo; 
};

using representation = std::vector<mesh>;

void setup_mesh(mesh &m) {
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glGenBuffers(1, &m.EBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, m.vertices.size() * sizeof(vertex), &m.vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.indices.size() * sizeof(unsigned int), &m.indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex_coords));
    glBindVertexArray(0);
}

void expand_bbox(std::vector<glm::vec3> &expanded_bbox, const glm::vec3 &min_bbox, const glm::vec3 &max_bbox) {
    expanded_bbox.clear();
    
    // Define the 8 corners of the bounding box
    glm::vec3 corners[8] = {
        glm::vec3(min_bbox.x, min_bbox.y, min_bbox.z), // 0: min corner
        glm::vec3(max_bbox.x, min_bbox.y, min_bbox.z), // 1
        glm::vec3(max_bbox.x, max_bbox.y, min_bbox.z), // 2
        glm::vec3(min_bbox.x, max_bbox.y, min_bbox.z), // 3
        glm::vec3(min_bbox.x, min_bbox.y, max_bbox.z), // 4
        glm::vec3(max_bbox.x, min_bbox.y, max_bbox.z), // 5
        glm::vec3(max_bbox.x, max_bbox.y, max_bbox.z), // 6: max corner
        glm::vec3(min_bbox.x, max_bbox.y, max_bbox.z)  // 7
    };
    
    // Define the 12 edges of the cube as line pairs
    int edges[12][2] = {
        // Bottom face edges
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        // Top face edges  
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        // Vertical edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    
    // Add line vertices
    for (int i = 0; i < 12; i++) {
        expanded_bbox.push_back(corners[edges[i][0]]);
        expanded_bbox.push_back(corners[edges[i][1]]);
    }
}

void setup_bounding_box(const glm::vec3 &min_bbox, const glm::vec3 &max_bbox, GLuint &vao, GLuint &vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    std::vector<glm::vec3> expanded_bbox;
    expand_bbox(expanded_bbox, min_bbox, max_bbox);
    // for (glm::vec3 bbox: expanded_bbox) print_vector(bbox);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, expanded_bbox.size() * sizeof(glm::vec3), &expanded_bbox[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);
}
void setup_model(model &mdl) {
    for (mesh &m: mdl.representation) {
        setup_mesh(m);
    }
}

unsigned int texture_from_memory(const aiTexture *texture) {
    GLuint id;
    int width;
    int height;
    int nr_components;
    // std::cout << texture->mFilename.C_Str() << std::endl;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    unsigned char *data = stbi_load_from_memory(
        reinterpret_cast<unsigned char*>(texture->pcData),
        texture->mWidth,
        &width,
        &height,
        &nr_components,
        0
    );
    if (nr_components == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else if (nr_components == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTextureParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

unsigned int texture_from_file(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;
    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    int width, height, nr_components;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nr_components, 0);
    if (data) {
        GLenum format;
        if (nr_components == 1)
            format = GL_RED;
        else if (nr_components == 3)
            format = GL_RGB;
        else if (nr_components == 4)
            format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return texture_id;
}

std::unordered_map<std::string, texture> load_material_textures(
    const aiScene *scene,
    aiMaterial *material,
    aiTextureType type,
    std::string type_name,
    const std::string& directory
    ) {
    std::unordered_map<std::string, texture>texture_map;
    for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        aiString str;
        material->GetTexture(type, i, &str);
        texture t;
        if (str.C_Str()[0] == '*') {
            int texture_index = std::stoi(str.C_Str() + sizeof(char));
            t.id = texture_from_memory(scene->mTextures[texture_index]);
        }
        t.type = type_name;
       t.path = str.C_Str();
        t.texture_unit = i;
        texture_map[t.path] = t;
    }
    return texture_map;
}

// Modified process_mesh to accept a transformation matrix
mesh process_mesh(aiMesh * assimp_mesh, const aiScene *scene, const std::string& directory, const glm::mat4& transform) {
    mesh m;
    
    // Extract the normal matrix (inverse transpose of the upper-left 3x3 of transform)
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
    
    for (unsigned int i = 0; i < assimp_mesh->mNumVertices; i++) {
        vertex v;
        
        // Apply transformation to position
        glm::vec4 pos = glm::vec4(assimp_mesh->mVertices[i].x, assimp_mesh->mVertices[i].y, assimp_mesh->mVertices[i].z, 1.0f);
        pos = transform * pos;
        v.position = glm::vec3(pos);
        
        // Apply normal matrix to normals
        if (assimp_mesh->HasNormals()) {
            glm::vec3 normal = glm::vec3(assimp_mesh->mNormals[i].x, assimp_mesh->mNormals[i].y, assimp_mesh->mNormals[i].z);
            v.normal = glm::normalize(normalMatrix * normal);
        }
        
        // Texture coordinates remain unchanged
        v.tex_coords = glm::vec2(0.0f, 0.0f);
        if (assimp_mesh->HasTextureCoords(0)) {
            v.tex_coords.x = assimp_mesh->mTextureCoords[0][i].x;
            v.tex_coords.y = assimp_mesh->mTextureCoords[0][i].y;
        }
        m.vertices.push_back(v);
    }
    
    // Indices remain unchanged
    for (unsigned int i = 0; i < assimp_mesh->mNumFaces; i++) {
        aiFace face = assimp_mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            m.indices.push_back(face.mIndices[j]);
        }
    }
    
    // Material processing remains unchanged
    if (assimp_mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[assimp_mesh->mMaterialIndex];
        std::unordered_map<std::string, texture> diffuse_maps = load_material_textures(
            scene, 
            material, 
            aiTextureType_DIFFUSE, 
            "texture_diffuse", 
            directory
        );
        m.textures.insert(diffuse_maps.begin(), diffuse_maps.end());
        std::unordered_map<std::string, texture> specular_maps = load_material_textures(scene, material, aiTextureType_SPECULAR, "texture_specular", directory);
        m.textures.insert(specular_maps.begin(), specular_maps.end());
    }
    return m;
}

void process_node(aiNode *node, const aiScene *scene, std::vector<mesh>& meshes, const std::string& directory, const glm::mat4& parentTransform = glm::mat4(1.0f)) {
    // Convert the node's transformation matrix and combine with parent transform
    glm::mat4 nodeTransform = aiMatrix4x4ToGlm(node->mTransformation);
    glm::mat4 globalTransform = parentTransform * nodeTransform;
    
    // Process all meshes in this node with the accumulated transformation
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(process_mesh(mesh, scene, directory, globalTransform));
    }
    
    // Recursively process child nodes, passing the accumulated transformation
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], scene, meshes, directory, globalTransform);
    }
}

void calculate_bounding_box(const std::vector<mesh>& meshes, glm::vec3& outMin, glm::vec3& outMax) {
    if (meshes.empty() || meshes[0].vertices.empty()) {
        outMin = glm::vec3(0.0f);
        outMax = glm::vec3(0.0f);
        return;
    }
    outMin = glm::vec3(FLT_MAX);
    outMax = glm::vec3(-FLT_MAX);
    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            outMin.x = std::min(outMin.x, vertex.position.x);
            outMin.y = std::min(outMin.y, vertex.position.y);
            outMin.z = std::min(outMin.z, vertex.position.z);
            outMax.x = std::max(outMax.x, vertex.position.x);
            outMax.y = std::max(outMax.y, vertex.position.y);
            outMax.z = std::max(outMax.z, vertex.position.z);
        }
    }
}

// Updated load_asset function (minimal changes needed)
std::vector<mesh> load_asset(const std::string& path) {
    Assimp::Importer importer;
    std::vector<mesh> meshes;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }
    std::string directory = path.substr(0, path.find_last_of('/'));
    
    // Start with identity matrix for root transformation
    process_node(scene->mRootNode, scene, meshes, directory, glm::mat4(1.0f));
    return meshes;
}

void load_models(const std::vector<std::string> paths, std::vector<model> &models) {
    for (std::string path: paths) {
        std::vector<mesh> representation = load_asset(path);
        glm::vec3 bb_lower;
        glm::vec3 bb_upper;
        calculate_bounding_box(representation, bb_lower, bb_upper);
        glm::vec3 centre = (bb_lower + bb_upper) / glm::vec3(2.0); 
        glm::vec3 position = glm::vec3(0);
        
        model mdl = {
            .representation = representation,
            .position = position,
            .bb_upper = bb_upper,
            .bb_lower = bb_lower,
            .bb_centre = centre,
        };
        
        // Set up bounding box VAO for this model
        setup_bounding_box(bb_lower, bb_upper, mdl.bbox_vao, mdl.bbox_vbo);
        models.push_back(mdl);
    }
}
struct shader {
    unsigned int id;
};

shader load_shader(const char * vertex_path, const char * fragment_path) {
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_file;
    std::ifstream fragment_file;
    vertex_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        vertex_file.open(vertex_path);
        fragment_file.open(fragment_path);
        std::stringstream vertex_stream, fragment_stream;
        vertex_stream << vertex_file.rdbuf();
        fragment_stream << fragment_file.rdbuf();
        vertex_file.close();
        fragment_file.close();
        vertex_code = vertex_stream.str();
        fragment_code = fragment_stream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char *v_shader_code = vertex_code.c_str();
    const char *f_shader_code = fragment_code.c_str();
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &v_shader_code, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &f_shader_code, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    GLuint id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return shader { .id = id };
}

void draw_bounding_box(const model &mdl, shader &s, const glm::vec3& position, const glm::mat4 &view, const glm::mat4 &projection) {
    glUseProgram(s.id);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    glUniformMatrix4fv(glGetUniformLocation(s.id, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(s.id, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(s.id, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    glBindBuffer(GL_ARRAY_BUFFER, mdl.bbox_vbo);
    std::vector<glm::vec3> expanded_bbox;
    expand_bbox(expanded_bbox, mdl.bb_lower, mdl.bb_upper);
    glBufferData(GL_ARRAY_BUFFER, expanded_bbox.size() * sizeof(glm::vec3), &expanded_bbox[0], GL_STATIC_DRAW);
    glBindVertexArray(mdl.bbox_vao);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

void draw_mesh(const mesh &m, shader &s) {
    unsigned int diffuse_nr = 1;
    unsigned int specular_nr = 1;
    for (auto &it: m.textures) {
        std::string texture_name = it.first;
        texture t = it.second;
        glActiveTexture(GL_TEXTURE0 + t.texture_unit);
        std::string uniform_name;
        if (t.type == "texture_diffuse") {
            uniform_name = "texture_diffuse1";
        }
        else if (t.type == "texture_specular") {
            uniform_name = "texture_specular1";
        };
        glUniform1i(glGetUniformLocation(s.id, uniform_name.c_str()), t.texture_unit);
        glBindTexture(GL_TEXTURE_2D, t.id);
    }
    glBindVertexArray(m.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(m.indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void draw_model(const model &mdl, shader &s) {
    glUseProgram(s.id);
    for (const mesh &msh: mdl.representation) {
        draw_mesh(msh, s);
    }
}

void draw_representation(const representation &r, shader &s) {
    for (const mesh &m: r) {
        draw_mesh(m, s);
    }
}

struct InputMap {
    bool    key_w;
    bool    key_a;
    bool    key_s;
    bool    key_d;
    bool    key_r;
    bool    key_f;
    bool    key_g;
    bool    mouse_left;
    float   mouse_x;
    float   mouse_y;
};

struct GameState {
    std::vector<model> models;

    std::vector<glm::mat4x4> transforms;
    std::vector<glm::vec3> min_aabb;
    std::vector<glm::vec3> max_aabb;
    std::vector<int> model_id;
    std::vector<int> entity_type;
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> displacement;
    
    int n;
};

struct EntityParameter {
    glm::mat4x4 transform;
    int model_id;
    int entity_type;
    glm::vec3 position;
    glm::vec3 displacement;
};

void create_entity(const EntityParameter &entity_parameter, GameState &game_state) {
    game_state.min_aabb.push_back(game_state.models.at(entity_parameter.model_id).bb_lower);
    game_state.max_aabb.push_back(game_state.models.at(entity_parameter.model_id).bb_upper);
    game_state.transforms.push_back(entity_parameter.transform);
    game_state.model_id.push_back(entity_parameter.model_id);
    game_state.entity_type.push_back(entity_parameter.entity_type);
    game_state.position.push_back(entity_parameter.position);
    game_state.displacement.push_back(entity_parameter.displacement);
}

/* Generates a test game state for experimentation */
void test_game_state(GameState &game_state) {
    load_models({"assets/dinosaur.glb", "bob_walking.fbx"}, game_state.models);
    // Set the player
    // glm::mat4x4 transform;
    // int model_id;
    // int entity_type;
    // glm::vec3 position;
    // glm::vec3 displacement;
 
    const EntityParameter player_parameter = {
        .transform  = glm::mat4x4(1.0),
        .model_id = 0,
        .entity_type = 0,
        .position = glm::vec3(0.0),
        .displacement = glm::vec3(0.0)
    };
    create_entity(player_parameter, game_state);
    // Set the bot
    const EntityParameter bot_parameter = {
        .transform = glm::mat4x4(1.0),
        .model_id = 0,
        .entity_type = 0,
        .position = glm::vec3(0.0),
        .displacement = glm::vec3(0.0)
    };
    create_entity(bot_parameter, game_state);
    game_state.n = game_state.model_id.size();
}

bool point_in_box(const glm::vec3 &point, const glm::vec3 &min_aabb, const glm::vec3 &max_aabb) {
    return (
        point.x >= min_aabb.x &&
        point.x <= max_aabb.x &&
        point.y >= min_aabb.y &&
        point.y <= max_aabb.y &&
        point.z >= min_aabb.z &&
        point.z <= max_aabb.z
    );
}

bool detect_collision(
    const glm::vec3 &min_aabb1,
    const glm::vec3 &max_aabb1,
    const glm::vec3 &min_aabb2,
    const glm::vec3 &max_aabb2
) {
    
    // std::cout << "---------" << std::endl;
    // std::cout << "min_aabb1" << std::endl;
    // print_vector(min_aabb1);
    // std::cout << "max_aabb1" << std::endl;
    // print_vector(max_aabb1);
    // std::cout << "min_aabb2" << std::endl;
    // print_vector(min_aabb2);
    // std::cout << "max_aabb2" << std::endl;
    // print_vector(max_aabb2);
    // std::cout << "---------" << std::endl;

    return (
        (min_aabb1.x <= max_aabb2.x && max_aabb1.x >= min_aabb2.x) &&
        (min_aabb1.y <= max_aabb2.y && max_aabb1.y >= min_aabb2.y) &&
        (min_aabb1.z <= max_aabb2.z && max_aabb1.z >= min_aabb2.z)
    );
}

void process_inputs(const InputMap &input_map, GameState &game_state) {
    auto wish_dir = glm::vec3(0.0);
    if (input_map.key_w) wish_dir += glm::vec3(0.0, 0.0, 0.05);
    if (input_map.key_a) wish_dir += glm::vec3(0.05, 0.0, 0.0);
    if (input_map.key_s) wish_dir += glm::vec3(0.0, 0.0, -0.05);
    if (input_map.key_d) wish_dir += glm::vec3(-0.05, 0.0, 0.0);
    if (input_map.key_f) wish_dir += glm::vec3(0.0, -0.05, 0.0);
    if (input_map.key_g) wish_dir += glm::vec3(0.0, 0.05, 0.0);
    game_state.position.at(0) += (wish_dir * glm::vec3(0.01));
    /*
    bool colliding = detect_collision(
        game_state.models[0].bb_lower,
        game_state.models[0].bb_upper,
        game_state.models[1].bb_lower,
        game_state.models[1].bb_upper
    );
    auto wish_dir = glm::vec3(0.0);
    if (input_map.key_w) wish_dir += glm::vec3(0.0, 0.0, 0.05);
    if (input_map.key_a) wish_dir += glm::vec3(-0.05, 0.0, 0.0);
    if (input_map.key_s) wish_dir += glm::vec3(0.0, 0.0, -0.05);
    if (input_map.key_d) wish_dir += glm::vec3(0.05, 0.0, 0.0);
    auto acceleration = glm::vec3(0, -9.8, 0);
    auto displacement = (acceleration * 0.016f) + (wish_dir * 3.0f);
    // std::cout << "------------" << std::endl;
    // std::cout << "displacement" << std::endl;
    // print_vector(displacement);
    // std::cout << "------------" << std::endl;
    if (colliding) {
        displacement.y = 0;
    }
    game_state.position[0] += displacement;
    game_state.displacement[0] += displacement;
    */
}

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW: Error initialising glfwInit\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    int width = 1024;
    int height = 768;
    std::string window_name = "Model Viewer";
    GLFWwindow* window = glfwCreateWindow(width, height, window_name.c_str(), NULL, NULL);
    if (window == NULL) {
        std::fprintf(stderr, "GLFW: Failed to create a window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    std::fprintf(stdout, "GLFW: Window creation successful\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return 1;
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    stbi_set_flip_vertically_on_load(false);
    GameState game_state;
    test_game_state(game_state);
    std::cout << "Loading shaders..." << std::endl;
    shader s = load_shader("./src/vert.glsl", "./src/frag.glsl");
    shader bbox_s = load_shader("./src/bboxvert.glsl", "./src/bboxfrag.glsl");
    for (model &mdl : game_state.models) {
        std::cout << "setting up" << std::endl;
        setup_model(mdl);
    }
    float fov = 45.0f;
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.0001f, 1000.0f);
    glm::vec3 cameraPos = glm::vec3(3.00f, 3.0f, 5.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // Look at the world origin
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, upVector);

    InputMap input_map;
    
    for (unsigned int model_id: game_state.model_id) {
        game_state.models.at(model_id).bb_lower += game_state.position[model_id];
        game_state.models.at(model_id).bb_upper += game_state.position[model_id];
    }
    while (!glfwWindowShouldClose(window)) {
        auto start_time = glfwGetTime();
        input_map.key_w = false;
        input_map.key_a = false;
        input_map.key_s = false;
        input_map.key_d = false;
        input_map.key_f = false;
        input_map.key_g = false;
        input_map.key_r = false;
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) input_map.key_w = true;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) input_map.key_a = true;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) input_map.key_s = true;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) input_map.key_d = true;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) input_map.key_f = true;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) input_map.key_g = true;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) input_map.key_r = true;
        glClearColor(0.44f, 0.57f, 0.74f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniformMatrix4fv(glGetUniformLocation(s.id, "view"), 1, GL_FALSE, &view[0][0]);
        glUniform3fv(glGetUniformLocation(s.id, "viewPos"), 1, &cameraPos[0]);
        glUniformMatrix4fv(glGetUniformLocation(s.id, "projection"), 1, GL_FALSE, &projection[0][0]);
        int curr_index = 0;
        process_inputs(input_map, game_state);
        for (int curr_entity = 0; curr_entity < game_state.n; curr_entity++) {
            glm::mat4x4 transform = glm::translate(glm::mat4x4(1.0f), game_state.position[curr_entity]);
            int model_id = game_state.model_id.at(curr_entity);
            game_state.models.at(model_id).bb_lower += game_state.displacement[model_id];
            game_state.models.at(model_id).bb_upper += game_state.displacement[model_id];
            glUniformMatrix4fv(glGetUniformLocation(s.id, "model"), 1, GL_FALSE, &transform[0][0]);
            draw_bounding_box(game_state.models.at(model_id), bbox_s, game_state.position[curr_entity], view, projection);
            draw_model(game_state.models.at(model_id), s);
            game_state.displacement[curr_entity] = glm::vec3(0);
            curr_index += 1;
        }
        glfwSwapBuffers(window);
        auto end_time = glfwGetTime();
        const auto frame_rate = std::format("{}", 1 / (end_time - start_time));
        glfwSetWindowTitle(window, frame_rate.c_str());
    }
    glfwTerminate();
    return 0;
}
