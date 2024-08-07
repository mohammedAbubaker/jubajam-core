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

struct RenderObject {
    std::vector<float> vertex_data;
    std::vector<unsigned char> texture_data;
    int width;
    int height;
};


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

    std::vector<RenderObject> draw_mesh() {
        std::vector<RenderObject> render_objects;
        // Will contain all the points
        std::vector<std::string> texture_names = { "l_legs", "h_head", "u_torso" };
        // texture name to image path
        std::unordered_map<std::string, std::string> texture_name_to_path;
        texture_name_to_path["l_legs"] = "../assets/futurefemale/legs1.tga";
        texture_name_to_path["h_head"] = "../assets/futurefemale/head1.tga";
        texture_name_to_path["u_torso"] = "../assets/futurefemale/torso1.tga";
        
        for (std::string texture_name : texture_names) {
            std::vector<float> vertex_data_buffer;
            // load texture data
            int width, height, nrChannels;
            unsigned char* data = stbi_load(
                    texture_name_to_path[texture_name].c_str(), 
                    &width, 
                    &height, 
                    &nrChannels, 
                    0
            );

            stbi_set_flip_vertically_on_load(true);

            std::vector<unsigned char> texture_data(data, data + (width*height*nrChannels));
            // load the corresponding vertex data
            std::vector<std::string> vertex_data = split_string_by_delimiter(
                "\n",
                read_file("../src/" + texture_name));

            for (std::string vertex : vertex_data) {
                // a vertex is a line in the form "x y z u v"
                std::vector<std::string> xyzuv = split_string_by_delimiter(
                    " ",
                    vertex);
                // Position coordinates
                vertex_data_buffer.push_back(std::stof(xyzuv.at(0)));
                vertex_data_buffer.push_back(std::stof(xyzuv.at(1)));
                vertex_data_buffer.push_back(std::stof(xyzuv.at(2)));
                // UV coordinates
                vertex_data_buffer.push_back(std::stof(xyzuv.at(3)));
                vertex_data_buffer.push_back(std::stof(xyzuv.at(4)));
            }

            // create a render object
            RenderObject render_object = RenderObject{
                .vertex_data = vertex_data_buffer, 
                .texture_data = texture_data,
                .width = width,
                .height = height,
            };
            render_objects.push_back(render_object);

            stbi_image_free(data);
        }
        return render_objects;
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

int nope()
{

    OBJ obj;

    RenderContext render_context;
    // obj.bind_buffer_data();
    if (!render_context.make_window(1024, 768, "Hello World")) {
        return -1;
    }

    GLuint vertex_array_id;
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);

    // Load vertex shader
    std::string vertex_shader_code
        = read_file("../src/vertex.glsl");
    std::string fragment_shader_code = read_file("../src/fragment.glsl");
    render_context.load_shaders(vertex_shader_code, fragment_shader_code);
    std::vector<RenderObject> render_objects = render_context.draw_mesh();

    glm::vec3 pos = glm::vec3(0.f, 20.f, -100.f);
    float viewing_angle = 90.f;
    
    // GLuint vao;
    std::vector<GLuint> vbos;
    std::vector<GLuint> txos;
    std::vector<int> sizes;
    int i = 0;

     
    // glCreateVertexArrays(1, &vao);

    for (RenderObject render_object: render_objects) {
        std::cout << render_object.texture_data.size() << std::endl;
        std::cout << render_object.width << std::endl;
        std::cout << render_object.height << std::endl;

        GLuint vbo;
        GLuint txo;

        glCreateBuffers(1, &vbo); 
        std::cout << vbo << std::endl;
        glNamedBufferStorage(
                vbo, 
                render_object.vertex_data.size() * sizeof(float),
                &render_object.vertex_data[0],
                GL_DYNAMIC_STORAGE_BIT
        );

        // glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 5);
        
        
        // glCreateTextures(GL_TEXTURE_2D, 1, &txo);
        glGenTextures(1, &txo);
        glBindTexture(GL_TEXTURE_2D, txo);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, render_object.width, render_object.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &render_object.texture_data[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
        // glBindTextureUnit(vbos.size(), txo);
        // glGenerateMipmap(GL_TEXTURE_2D);

        vbos.push_back(vbo);
        txos.push_back(txo);
        sizes.push_back(render_object.vertex_data.size());
        // glDisable(GL_TEXTURE_2D);
        //
               
    }
    
    /*
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    */


       
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
    // std::cout << g_vertex_buffer_data.size() << std::endl;
    //
    //               
    // glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);

    int sum_of_vertices = 0;
    for (int s: sizes) {
        sum_of_vertices += s;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    float current_angle = 0;
    while (!glfwWindowShouldClose(render_context.window)) {
        render_context.load_mvp(pos, glm::radians(viewing_angle));
        render_context.rotate_model(current_angle);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glBindTexture(GL_TEXTURE_2D, txos.at(0));

        for (int i = 0; i < vbos.size(); i++) {
            glBindBuffer(GL_ARRAY_BUFFER, vbos.at(i));
            glBindTexture(GL_TEXTURE_2D, txos.at(i));
            glGetError();
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glDrawArrays(GL_TRIANGLES, 0, sizes.at(i) / 3);
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

    GLTFfile ob2j;
    ob2j.lol();

    return 0;
}

