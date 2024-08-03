#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include <cstdio>
#include <filesystem>
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <sstream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "unordered_map"
#include <nlohmann/json.hpp>

#include <vector>

struct TextureObject {
    int width;
    int height;
    int nrChannels;
    std::vector<unsigned char> texture_data;
};

struct VertexObject {
    float x;
    float y;
    float z;
    float u;
    float v;
};

std::string read_file(std::string file_path)
{
    std::ifstream file(file_path.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> split_string_by_delimiter(std::string delimiter, std::string input_string)
{
    std::string substring = "";
    std::vector<std::string> result;
    int i = 0;
    while (i < input_string.length()) {
        std::string token = input_string.substr(i, delimiter.length());
        if (token == delimiter) {
            result.push_back(substring);
            substring = "";
            i += delimiter.size();
            continue;
        }
        substring += input_string[i];
        i += 1;
    }

    if (!substring.empty()) {
        result.push_back(substring);
    }

    return result;
}

struct RenderContext {
    GLuint program_id;

    GLuint matrix_id;

    glm::mat4 mvp;

    GLFWwindow* window;

    /*
     * Makes a window and returns its success.
     *
     * @param width : Width of the window in pixels.
     * @param height : Height of the window in pixels.
     * @param name : Name of the window.
     *
     * @return : True -> Window created successfull.
     * @return : False -> No window created due to error.
     * */

    bool make_window(int width, int height, std::string name)
    {
        if (!glfwInit()) {
            std::fprintf(stderr, "GLFW: Error initialising glfwInit\n");
            return false;
        }

        this->window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

        if (window == NULL) {
            std::fprintf(stderr, "GLFW: Failed to create a window\n");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(this->window);
        std::fprintf(stderr, "GLFW: Window creation successful\n");

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1;
        }

        return true;
    }

    void load_mvp(glm::vec3 pos, float viewing_angle)
    {
        // Projection matrix
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)1024 / (float)768, 0.1f, 100.0f);

        // Camera matrix
        glm::vec3 unit_vector = glm::vec3(cos(viewing_angle), 0.0f, sin(viewing_angle));
        glm::mat4 view
            = glm::lookAt(pos, pos + unit_vector, glm::vec3(0, 1, 0));

        // Model matrix (identity for now, but will be different for each model)
        glm::mat4 model = glm::mat4(1.0f);

        // Model view projection matrix
        glm::mat4 mvp = projection * view * model;

        // set the mvp
        this->mvp = mvp;
        // Get a handle for the matrix
        this->matrix_id = glGetUniformLocation(this->program_id, "MVP");
    }

    void apply_mvp()
    {
        // std::cout << glm::to_string(this->mvp) << std::endl;
        glUniformMatrix4fv(this->matrix_id, 1, GL_FALSE, &this->mvp[0][0]);
    }

    void load_shaders(std::string vertex_shader_code, std::string fragment_shader_code)
    {
        GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

        GLint result = GL_FALSE;
        GLint info_log_length;

        // Compile the vertex shader
        std::cout << "Compiling vertex shader" << std::endl;
        char const* vertex_source_pointer = vertex_shader_code.c_str();
        glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, NULL);
        glCompileShader(vertex_shader_id);

        // Validate vertex shader
        glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

        // Compile fragment shader
        std::cout << "Compiling fragment shader" << std::endl;
        char const* fragment_source_pointer = fragment_shader_code.c_str();
        glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, NULL);
        glCompileShader(fragment_shader_id);

        // Validate fragment shader
        glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
        if (info_log_length > 0) {
            std::vector<char> fragment_shader_error_message(info_log_length + 1);
            glGetShaderInfoLog(
                fragment_shader_id, info_log_length, NULL, &fragment_shader_error_message[0]);
            std::printf("%s\n", &fragment_shader_error_message[0]);
        }

        // Link the program
        std::cout << "Linking the program" << std::endl;
        this->program_id = glCreateProgram();
        glAttachShader(this->program_id, vertex_shader_id);
        glAttachShader(this->program_id, fragment_shader_id);
        glLinkProgram(this->program_id);

        // Check the program
        glGetProgramiv(this->program_id, GL_LINK_STATUS, &result);
        glGetProgramiv(this->program_id, GL_INFO_LOG_LENGTH, &info_log_length);
        if (info_log_length > 0) {
            std::vector<char> program_error_message(info_log_length + 1);
            glGetProgramInfoLog(this->program_id, info_log_length, NULL, &program_error_message[0]);
            std::printf("%s\n", &program_error_message[0]);
        }

        glDetachShader(this->program_id, vertex_shader_id);
        glDetachShader(this->program_id, fragment_shader_id);

        glDeleteShader(vertex_shader_id);
        glDeleteShader(fragment_shader_id);

        glUseProgram(this->program_id);
    }

    void update_screen()
    {
        glfwSwapBuffers(this->window);
        glfwPollEvents();
    }

    std::vector<float> draw_mesh() {
        // Will contain all the points
        std::vector<float> vertex_data_buffer;
        std::vector<std::string> texture_names = { "l_legs", "h_head", "u_torso" };
        for (std::string texture_name : texture_names) {
            // load the corresponding vertex data
            std::vector<std::string> vertex_data = split_string_by_delimiter(
                "\n",
                read_file("../src/" + texture_name));

            for (std::string vertex : vertex_data) {
                std::cout << vertex << std::endl;
                // a vertex is a line in the form "x y z u v"
                std::vector<std::string> xyzuv = split_string_by_delimiter(
                    " ",
                    vertex);

                vertex_data_buffer.push_back(std::stof(xyzuv.at(0)));
                vertex_data_buffer.push_back(std::stof(xyzuv.at(1)));
                vertex_data_buffer.push_back(std::stof(xyzuv.at(2)));

                // vertex_data_buffer.push_back(std::stof(xyzuv.at(3)));
                // vertex_data_buffer.push_back(std::stof(xyzuv.at(4)));
            }
        }
        return vertex_data_buffer;
    }
};



struct FaceElement {
    glm::vec3 vertex;
    glm::vec3 normal;
    glm::vec2 texture;
};

struct Face {
    FaceElement face_element_0;
    FaceElement face_element_1;
    FaceElement face_element_2;
};

struct OBJ {

    std::vector<glm::vec3> vertex_set;
    std::vector<glm::vec3> normal_set;
    std::vector<glm::vec2> texture_set;

    std::vector<Face> face_set;

    void print_face_element(FaceElement face_element)
    {
        std::cout << "Printing texture: " << glm::to_string(face_element.texture) << std::endl;
        std::cout << "Printing normal: " << glm::to_string(face_element.normal) << std::endl;
        std::cout << "Printing vertex: " << glm::to_string(face_element.vertex) << std::endl;
    }

    void print_face(Face face)
    {
        std::cout << "Printing face: " << std::endl;

        std::cout << "Printing face element 0:" << std::endl;
        this->print_face_element(face.face_element_0);
        std::cout << "Printing face element 1:" << std::endl;
        this->print_face_element(face.face_element_1);
        std::cout << "Printing face element 2:" << std::endl;
        this->print_face_element(face.face_element_2);
    }

    void process_vertices(std::string line)
    {
        std::vector<std::string> vertices = split_string_by_delimiter(" ", line);
        float x = std::stof(vertices.at(1));
        float y = std::stof(vertices.at(2));
        float z = std::stof(vertices.at(3));

        this->vertex_set.push_back(glm::vec3(x, y, z));
    }

    void process_normals(std::string line)
    {
        std::vector<std::string> normals = split_string_by_delimiter(" ", line);
        float x = std::stof(normals.at(1));
        float y = std::stof(normals.at(2));
        float z = std::stof(normals.at(3));

        this->normal_set.push_back(glm::vec3(x, y, z));
    }

    void process_textures(std::string line)
    {
        std::vector<std::string> textures = split_string_by_delimiter(" ", line);
        float u = std::stof(textures.at(1));
        float w = std::stof(textures.at(2));

        this->texture_set.push_back(glm::vec2(u, w));
    }

    /*
     * Currently, I'm trying to pass all the vertex and texture data into seperate classes,
     * And then proccessing those classes in OpenGL.
     * This approach is probably not a good idea, and I am trying to think of think up a
     * data oriented solution, that could parse a bunch of different variations.
     *
     * The current plan is to:
     *  1. Generate and fill a vertex buffer array.
     *  2. Generate and fill an index buffer array.
     *  3. Generate and fill a texture buffer array.
     *  4. Generate and fill an index buffer array for the texture buffer?
     *  From the plan, it seems like I should learn how tf index buffers work...
     * */

    // TODO Think of a better naming scheme...
    std::vector<float> process_faces(std::string line)
    {
        std::vector<std::string> face_elements = split_string_by_delimiter(" ", line);
        // Drop the "f" element of the string, not needed.
        face_elements.erase(face_elements.begin());

        std::vector<FaceElement> face_element_container;

        std::vector<float> vertices;

        for (std::string face_element_str : face_elements) {
            // A face_element is in the form: vertex_index//normal_index//texture_index.
            std::vector<std::string> face_elements = split_string_by_delimiter("/", face_element_str);

            size_t vertex_index = std::stoi(face_elements.at(0));
            vertices.push_back(this->vertex_set.at(vertex_index - 1).x);
            vertices.push_back(this->vertex_set.at(vertex_index - 1).y);
            vertices.push_back(this->vertex_set.at(vertex_index - 1).z);
            /*
            size_t texture_index = std::stoi(face_elements.at(1));
            size_t normal_index = std::stoi(face_elements.at(2));

            FaceElement {
            face_element_container.push_back(
                    .vertex = this->vertex_set.at(vertex_index - 1),
                    .normal = this->normal_set.at(normal_index - 1),
                    .texture = this->texture_set.at(texture_index - 1) });
            */
        }

        /*
        this->face_set.push_back(Face {
            .face_element_0 = face_element_container.at(0),
            .face_element_1 = face_element_container.at(1),
            .face_element_2 = face_element_container.at(2) });

        this->print_face(this->face_set.at(this->face_set.size() - 1));
        */
        return vertices;
    }

    std::vector<float> process_line(std::string line)
    {
        std::vector<float> vertices;

        std::string identifier = line.substr(0, 2);
        if (identifier == "vn") {
            process_normals(line);
        }
        if (identifier == "vt") {
            process_textures(line);
        }
        if (identifier == "v ") {
            process_vertices(line);
        }
        if (identifier == "f ") {
            std::vector<float> verts = process_faces(line);
            for (float v : verts) {
                vertices.push_back(v);
            }
        }

        return vertices;
    }
};

int main()
{

    OBJ obj;

    RenderContext render_context;
    // obj.bind_buffer_data();
    if (!render_context.make_window(1024, 768, "Hello World")) {
        return -1;
    }
    // Load vertex shader
    std::string vertex_shader_code
        = read_file("../src/vertex.glsl");
    std::string fragment_shader_code = read_file("../src/fragment.glsl");
    render_context.load_shaders(vertex_shader_code, fragment_shader_code);

    glm::vec3 pos = glm::vec3(3.f, 4.f, 5.f);
    float viewing_angle = 3.f;
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    std::vector<float> g_vertex_buffer_data = render_context.draw_mesh();
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), &g_vertex_buffer_data[0], GL_STATIC_DRAW);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    std::cout << g_vertex_buffer_data.size() << std::endl;

    while (!glfwWindowShouldClose(render_context.window)) {
        render_context.load_mvp(pos, glm::radians(viewing_angle));
        glClear(GL_COLOR_BUFFER_BIT);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 
            3, 
            GL_FLOAT, 
            GL_FALSE, 
            3 * sizeof(float), 
            (void*)0
        );
        glDrawArrays(GL_TRIANGLES, 0, g_vertex_buffer_data.size() / 3);
        glDisableVertexAttribArray(0);
        int state = glfwGetKey(render_context.window, GLFW_KEY_W);
        if (state == GLFW_PRESS) {
            pos += glm::vec3(0.1f, 0.0f, 50.0f);
        }

        state = glfwGetKey(render_context.window, GLFW_KEY_S);
        if (state == GLFW_PRESS) {
            pos -= glm::vec3(0.1f, 0.0f, 0.0f);
        }

        state = glfwGetKey(render_context.window, GLFW_KEY_D);
        if (state == GLFW_PRESS) {
            pos += glm::vec3(0.0f, 0.0f, 0.1f);
        }

        state = glfwGetKey(render_context.window, GLFW_KEY_A);
        if (state == GLFW_PRESS) {
            pos -= glm::vec3(0.0f, 0.0f, 0.1f);
        }

        state = glfwGetKey(render_context.window, GLFW_KEY_LEFT);
        if (state == GLFW_PRESS) {
            viewing_angle -= 2.f;
        }

        state = glfwGetKey(render_context.window, GLFW_KEY_RIGHT);
        if (state == GLFW_PRESS) {
            viewing_angle += 2.f;
        }
        render_context.apply_mvp();
        render_context.update_screen();
    }
    return 0;
}
