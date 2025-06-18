#include <fstream>
#include <glad/glad.h>
#include <GL/gl.h>
#include <assimp/Importer.hpp>
#include <GLFW/glfw3.h>
#include <assimp/material.h>
#include <sstream>
#include <thread>

#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <string>

#include <chrono>
#include <cstdlib>

#include <vector>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};


struct texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct mesh {
    std::vector<vertex>         vertices;
    std::vector<unsigned int>   indices;
    std::vector<texture>        textures;
};

void setup_mesh(mesh m, GLuint vao, GLuint vbo, GLuint ebo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, m.vertices.size() * sizeof(vertex), &m.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.indices.size() * sizeof(unsigned int), &m.indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex_coords));

    // Unbind
    glBindVertexArray(0);
}

unsigned int texture_from_file(const char *path, const std::string directory) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;
    unsigned int texture_id;
    glGenTextures(1, &texture_id);

    int width, height, nr_components;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nr_components, 0);
    if (data) {
        GLenum format;
        if (nr_components == 1) {
            format = GL_RED;
        }
        else if (nr_components == 3) {
            format = GL_RGB;
        }

        else if (nr_components == 4) {
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

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

std::vector<texture> load_material_textures(
    aiMaterial *material, 
    aiTextureType type, 
    std::string type_name, 
    std::string directory
    ) {
    std::vector<texture> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        aiString str;
        material->GetTexture(type, i, &str);
        texture t;
        t.id = texture_from_file(str.C_Str(), directory);
        t.type = type_name;
        t.path = str.C_Str();
        textures.push_back(t);
    }
    return textures;
}

mesh process_mesh(aiMesh * assimp_mesh, const aiScene *scene, std::string directory) {
    mesh m = {
        .vertices   = {},
        .indices    = {},
        .textures   = {},
    };
    for (unsigned int i = 0; i < assimp_mesh->mNumVertices; i++) {
        vertex v;
        v.position.x    = assimp_mesh->mVertices[i].x;
        v.position.y    = assimp_mesh->mVertices[i].y;
        v.position.z    = assimp_mesh->mVertices[i].z;
        v.normal.x  = assimp_mesh->mNormals[i].x;
        v.normal.y  = assimp_mesh->mNormals[i].y;
        v.normal.z  = assimp_mesh->mNormals[i].z;
        v.tex_coords = glm::vec2(0.0f, 0.0f);
        if (assimp_mesh->mTextureCoords[0]) {
            v.tex_coords.x = assimp_mesh->mTextureCoords[0][i].x;
            v.tex_coords.y = assimp_mesh->mTextureCoords[0][i].y;
        }
        m.vertices.push_back(v);
    }
    for (unsigned int i = 0; i < assimp_mesh->mNumFaces; i++) {
        aiFace face = assimp_mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            m.indices.push_back(face.mIndices[j]);
        }
    }
    if (assimp_mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[assimp_mesh->mMaterialIndex];
        std::vector<texture> diffuse_maps = load_material_textures(
            material, 
            aiTextureType_DIFFUSE, 
            "texture_diffuse", 
            directory
        );
        m.textures.insert(m.textures.end(), diffuse_maps.begin(), diffuse_maps.end());
        std::vector<texture> specular_maps = load_material_textures(
            material,
            aiTextureType_SPECULAR,
            "texture_specular",
            directory
        );
        m.textures.insert(m.textures.end(), specular_maps.begin(), specular_maps.end());
    }

    return m;
};

std::vector<mesh> process_node(aiNode *node, const aiScene *scene, std::string directory) {
    std::vector<mesh> meshes;
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(process_mesh(mesh, scene, directory));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        std::vector<mesh> associated_nodes = process_node(node->mChildren[i], scene, directory);
        meshes.insert(meshes.end(), associated_nodes.begin(), associated_nodes.end());
    }
    return meshes;
}

std::vector<mesh> load_assets(std::vector<std::string> asset_paths) {
    Assimp::Importer importer;
    std::vector<mesh> meshes;
    for (std::string path: asset_paths) {
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
        std::string directory = path.substr(0, path.find_last_of('/'));
        std::vector<mesh> associated_meshes = process_node(scene->mRootNode, scene, directory);
        meshes.insert(meshes.end(), associated_meshes.begin(), associated_meshes.end());
    }
    return meshes;
}

struct shader {
    unsigned int id;
};

shader load_shader(const char * vertex_path, const char * fragment_path) {
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_file;
    std::ifstream fragment_file;
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
    // Compile shaders
    unsigned int vertex, fragment;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &v_shader_code, NULL);
    glCompileShader(vertex);
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &f_shader_code, NULL);
    glCompileShader(fragment);
    // Link shaders
    GLuint id;
    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return shader {
        .id = id
    };
}

void draw_mesh(mesh m, shader s) {
    unsigned int diffuse_nr = 1;
    unsigned int specular_nr = 1;
    for (unsigned int i = 0; i < m.textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        std::string number;
        std::string name = m.textures[i].type;
        if (name == "texture_diffuse") {
            number = std::to_string(diffuse_nr++);
        }
        else if (name == "texture_specular") {
            number = std::to_string(specular_nr++);
        }
    }
}

int main() {
    std::vector<std::string> asset_paths = {
        "../assets/BrainStem.glb"
    };
    std::cout << "Loading shaders..." << std::endl;;
    shader s = load_shader("frag.glsl", "vert.glsl");
    std::cout << "Shaders loaded." << std::endl;
    std::cout << "Loading meshes..." << std::endl;
    std::vector<mesh> meshes = load_assets(asset_paths);
    std::cout << "Successfully loaded " << meshes.size() << " meshes" << std::endl;
    for (mesh m : meshes) {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        setup_mesh(m, vao, vbo, ebo);
        draw_mesh(m, s);
    }
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW: Error initialising glfwInit\n");
        return 1;
    }
    int width = 1024;
    int height = 768;
    std::string window_name = "goodbye";
    auto window = glfwCreateWindow(width, height, window_name.c_str(), NULL, NULL);
    if (window == NULL) {
        std::fprintf(stderr, "GLFW: Failed to create a window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    std::fprintf(stderr, "GLFW: Window creation successful\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return 1;
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}
