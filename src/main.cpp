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

#include "unordered_map"
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>

#include <vector>
#include "parser.h"

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

    glm::mat3 rot;

    GLuint rot_id;

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

    void rotate_model(double angle) {
        std::chrono::milliseconds ms = duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        );
        float current_ms = (ms.count() % 60);
        std::cout << current_ms << std::endl;
        
        // take 5 seconds to do 1 360... 
        angle += (current_ms * 3);
        if (angle >= 360.0) {
            angle = 0.0;
        }

        // convert to radians
        double angle_radians = glm::radians(angle);
        angle_radians = 0;
        glm::mat4 rot = glm::mat3(
            std::cos(angle_radians),    0,  std::sin(angle_radians),
            0,                          1,  0,
            -std::sin(angle_radians),   0,  std::cos(angle_radians)
        );
        this->rot = rot;
        this->rot_id = glGetUniformLocation(this->program_id, "ROT");
    }

    void apply_rot()
    {
        glUniformMatrix3fv(this->rot_id, 1, GL_FALSE, &this->rot[0][0]);
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

std::unordered_map<std::string, std::vector<GLuint>> initialise_mesh(std::string path) {
    std::unordered_map<std::string, std::vector<GLuint>> result;
    ParseModel parse_model;

    result["vbo"] = std::vector<GLuint>();
    result["txo"] = std::vector<GLuint>();
    result["sizes"] = std::vector<GLuint>();

    std::vector<RenderPart> render_parts = parse_model.parse_model(path);
    for (RenderPart render_part: render_parts) {
        // Create vertex data
        std::vector<float> vertex_data;
        int i = 0;
        int j = 0;
        while ((i + j) < (render_part.position_data.size() + render_part.texcoord_data.size())) {
            vertex_data.push_back(render_part.position_data.at(i));
            vertex_data.push_back(render_part.position_data.at(i+1));
            vertex_data.push_back(render_part.position_data.at(i+2));

            vertex_data.push_back(render_part.texcoord_data.at(j));
            vertex_data.push_back(render_part.texcoord_data.at(j+1));

            i += 3;
            j += 2;
        }

        // Set the vertex data
        GLuint vbo;
        GLuint txo;

        glCreateBuffers(1, &vbo);
        glNamedBufferStorage(
                vbo,
                vertex_data.size() * sizeof(float),
                &vertex_data[0],
                GL_DYNAMIC_STORAGE_BIT
        );

        glGenTextures(1, &txo);
        glBindTexture(GL_TEXTURE_2D, txo);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGB,
                render_part.texture_info.width,
                render_part.texture_info.height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                &render_part.texture_info.texture_data[0]
        );
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_S, GL_REPEAT
        );
        glTexParameteri(
                GL_TEXTURE_2D, 
                GL_TEXTURE_WRAP_T, 
                GL_REPEAT
        );
        glTexParameteri(
                GL_TEXTURE_2D, 
                GL_TEXTURE_MIN_FILTER, 
                GL_LINEAR_MIPMAP_LINEAR
        );
        glTexParameteri(
                GL_TEXTURE_2D, 
                GL_TEXTURE_MAG_FILTER, 
                GL_LINEAR
        );

        result["sizes"].push_back(render_part.position_data.size());
    }
    return result;
}

int main()
{
    RenderContext render_context;
    if (!render_context.make_window(1024, 768, "Hello World")) {
        return -1;
    }
    // Load vertex shader
    std::string vertex_shader_code
        = read_file("../src/vertex.glsl");
    std::string fragment_shader_code = read_file("../src/fragment.glsl");
    render_context.load_shaders(vertex_shader_code, fragment_shader_code);
    glm::vec3 pos = glm::vec3(0.f, 20.f, -100.f);
    float viewing_angle = 90.f;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    std::unordered_map<std::string, std::vector<GLuint>> bindings = initialise_mesh("../assets/Monster.gltf");
    while (!glfwWindowShouldClose(render_context.window)) {
        // render_context.load_mvp(pos, glm::radians(viewing_angle));
        // render_context.rotate_model(current_angle);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glBindTexture(GL_TEXTURE_2D, txos.at(0));
        for (int i = 0; i < bindings["vbo"].size(); i++) {
            glBindBuffer(GL_ARRAY_BUFFER, bindings["vbos"].at(i));
            glBindTexture(GL_TEXTURE_2D, bindings["txos"].at(i));
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glDrawArrays(GL_TRIANGLES, 0, bindings["sizes"].at(i) / 3);
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
        }
        // glBindVertexArray(vao);
        // // glDrawArrays(GL_TRIANGLES, 0, sum_of_vertices / 3);
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
        render_context.apply_rot();
        render_context.update_screen();
    }
    return 0;
}
