#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_bool2_precision.hpp"
#include "glm/trigonometric.hpp"
#include <cstdio>
#include <filesystem>
#include <glad/glad.h>

#include <tinygltf/tiny_gltf.h>

#include <GLFW/glfw3.h>
#include <fstream>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>

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
#include <cameron314/atomicops.h>
#include <cameron314/readerwriterqueue.h>

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

// ------------------------------------------------------------------ //

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

    void rotate_model(double angle)
    {
        std::chrono::milliseconds ms = duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
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
            std::cos(angle_radians), 0, std::sin(angle_radians),
            0, 1, 0,
            -std::sin(angle_radians), 0, std::cos(angle_radians));
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
        if (info_log_length > 0) 
        {
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

std::mutex game_state_mutex;
std::mutex key_map_mutex;
std::hash<std::string> hasher;

enum Entity
{
    PLAYER_ID,
    ARENA_ID,
    HEALTHBAR_ID,
    AI_ID
};


struct Mobs
{
    std::vector<Entity> v_id;
    std::vector<float> v_health;
    std::vector<glm::vec3> v_pos;
    std::vector<std::string> v_repr;
    std::vector<float> strength_modifier;
};

struct Scenes
{
    std::vector<Entity> v_id;
    std::vector<glm::vec3> v_pos;
};


// User Interface Element(s)
struct UIEs
{
    std::vector<Entity> v_id;
    std::vector<std::string> v_repr;
};

struct GameState 
{
    Mobs mobs;
    Scenes scenes;
    UIEs ui;
};

struct KeyState 
{
    bool W;
    bool A;
    bool S;
    bool D;
    bool LEFT;
    bool RIGHT;
};

GameState initialise_game_state()
{
    Mobs mobs
    (
        {PLAYER_ID, AI_ID}, // v_id
        {0.0, 0.0}, // v_health;
        {glm::vec3(0.0, 0.0, 1.0), glm::vec3(1.0, 1.0, 0.0)}, // v_pos;
        {"player.gltf", "ai.gltf"}, // v_repr;
        {1.0, 1.0}  // strength_modifier;
    );

    Scenes scenes
    (
        {ARENA_ID}, // v_id
        {glm::vec3(0.0,0.0,0.0)} // v_pos
    );

    UIEs uies
    (
        {HEALTHBAR_ID}, // v_id
        {"healthbar.gltf"} // v_repr
    );

    return GameState
    (
        mobs,
        scenes,
        uies
    );

}
KeyState initialise_key_state()
{
    return 
    {
        false,
        false,
        false,
        false
    };
};

GameState game_state = initialise_game_state();
std::map<std::atomic<int>, std::atomic<bool>> key_map;

moodycamel::ReaderWriterQueue<int> key_queue(100);

struct MeshData
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uv_coords;
};

tinygltf::Model load_gltf(std::string path, tinygltf::TinyGLTF &loader)
{
    tinygltf::Model model;
    std::vector<MeshData> mesh_data;
    std::string warn;
    std::string err;

    loader.LoadASCIIFromFile
    (
        &model,
        &err,
        &warn,
        path.c_str()
    );

    return model;
};

std::vector<MeshData> load_mesh_data(const std::vector<std::string> &reprs)
{
    std::vector<MeshData> result;
    tinygltf::TinyGLTF loader;
    for (std::string repr : reprs)
    {
        tinygltf::Model model = load_gltf(repr, loader);
        for (size_t i = 0; i < model.bufferViews.size(); i++)
        {
            tinygltf::BufferView buffer_view = model.bufferViews[i];
            if (buffer_view.target == 0) continue;
            tinygltf::Buffer buffer = model.buffers[buffer_view.buffer];
            std::cout << buffer.name << std::endl;
        }
    }
    return result;
}

struct MaterialData
{
};

void allocate_memory()
{
    // Returns memory description of the GPU:
    // vertices region:
    // texture region:
    // uv coord region:
    // norm region:
    // animation region:
};

enum ARRAYS
{
    VERTICES,
    INDICES,
    TEXTURES,
};


GLuint add_shader(std::string shader_path, GLuint type)
{
    GLuint shader_id;
    int success;
    char info_log[512];

    std::string source = read_file(shader_path);
    const char *source_pointer =source.c_str();
    shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source_pointer, NULL);
    glCompileShader(shader_id);
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader_id, 512, NULL, info_log);
        std::cout << "ERROR: " << info_log << std::endl;
    }
    else
    {
        std::cout << "Successfully added: " << shader_path << std::endl;
    }

    return shader_id;
}

struct RenderData
{
    GLuint vao = 0;
    GLuint vertex_vbo = 0;
    GLuint colour_vbo = 0;
    GLuint index_ebo = 0;
    size_t index_count = 0;
};

std::vector<RenderData> initialise_render()
{
    RenderData render_data;
    std::vector<glm::vec3>  vertices = 
    {
        glm::vec3(-0.5,-0.5,1.1), 
        glm::vec3(0.5,-0.5,0.0), 
        glm::vec3(0.0,0.5,0.0)
    };

    std::vector<glm::vec4> colours = 
    {
        glm::vec4(1.0,0.0,0.0, 1.0), 
        glm::vec4(1.0,0.0,0.0, 1.0), 
        glm::vec4(1.0,0.0,0.0, 1.0)
    };

    std::vector<unsigned int> indices = {0, 1, 2};
    render_data.index_count = indices.size();

    glCreateBuffers(1, &render_data.vertex_vbo);
    glNamedBufferData(
        render_data.vertex_vbo, 
        vertices.size() * sizeof(glm::vec3), 
        vertices.data(), 
        GL_DYNAMIC_DRAW
    );

    glCreateBuffers(1, &render_data.colour_vbo);
    glNamedBufferData(
        render_data.colour_vbo, 
        colours.size() * sizeof(glm::vec4), 
        colours.data(), 
        GL_DYNAMIC_DRAW
    );

    glCreateBuffers(1, &render_data.index_ebo);
    glNamedBufferData(
        render_data.index_ebo,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_DYNAMIC_DRAW
    );

    glVertexArrayVertexBuffer(
        render_data.vao,
        0,
        render_data.vertex_vbo,
        0,
        sizeof(glm::vec3)
    );
    glEnableVertexArrayAttrib(
        render_data.vao,
        0
    );
    glVertexArrayAttribFormat(
        render_data.vao,
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        0
    );
    glVertexArrayAttribBinding(
        render_data.vao,
        0, 
        0
    );

    glVertexArrayVertexBuffer(
        render_data.vao,
        1,
        render_data.colour_vbo,
        0,
        sizeof(glm::vec4)
    );
    glEnableVertexArrayAttrib(
        render_data.vao,
        1
    );
    glVertexArrayAttribFormat(
        render_data.vao,
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        0
    );
    glVertexArrayAttribBinding(
        render_data.vao,
        1,
        1
    );

    glVertexArrayElementBuffer(
        render_data.vao,
        render_data.index_ebo
    );

    return {render_data};
}

GLuint create_program(std::vector<GLuint> shaders)
{
    int success;
    GLuint program;
    char info_log[512];

    program = glCreateProgram();
    for (GLuint shader: shaders)
    {
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        std::cout << "Error: " << info_log << std::endl;
    }
    else
    {
        std::cout << "Successfully compiled program id " << program << std::endl;
    }

    for (GLuint shader: shaders)
    {
        glDeleteShader(shader);
    }

    return program;
}

void draw(std::vector<RenderData> render_data)
{
    for (RenderData render_datum: render_data)
    {
        glBindVertexArray(render_datum.vao);
        glDrawElements(
            GL_TRIANGLES, 
            render_datum.index_count,
            GL_UNSIGNED_INT,
            nullptr
        );
    }
}



void render(std::stop_token stop_token)
{
    std::vector<GLuint> shaders = 
    {
        add_shader("vert.glsl", GL_VERTEX_SHADER), 
        add_shader("frag.glsl", GL_FRAGMENT_SHADER)
    };

    GLuint program = create_program(shaders);

    std::vector<RenderData> render_data = initialise_render();
    int wait_ms = 16;
    while (!stop_token.stop_requested()) 
    {
        glUseProgram(program);
        draw(render_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    }
}

void translate_player(GameState &game_state, glm::vec3 vector)
{
    int index = 0;
    for (Entity entity_id : game_state.mobs.v_id)
    {
        if (entity_id == PLAYER_ID)
        {
            game_state.mobs.v_pos[index] += vector;
        }
    }
    index += 1;
}

void game(std::stop_token stop_token)
{
    std::jthread render_thread(render);
    int wait_ms = 1;

    std::map<int, bool> key_map;

    while (!stop_token.stop_requested()) 
    {
        int key;
        if (key_queue.try_dequeue(key))
        {
            if (key == GLFW_KEY_W) key_map[GLFW_KEY_W] = true;
            if (key == GLFW_KEY_A) key_map[GLFW_KEY_A] = true;
            if (key == GLFW_KEY_S) key_map[GLFW_KEY_S] = true;
            if (key == GLFW_KEY_D) key_map[GLFW_KEY_D] = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        {
            std::lock_guard<std::mutex> lock(game_state_mutex);
            if (key_map[GLFW_KEY_W]) translate_player(game_state, glm::vec3(0,0,1));
            if (key_map[GLFW_KEY_A]) translate_player(game_state, glm::vec3(0,1,0));
        }

        // Reset key map
        key_map.clear();
    }
}

int main()
{

    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW: Error initialising glfwInit\n");
        return 1;
    }

    int width = 1024;
    int height = 768;
    std::string window_name = "goodbye";
    auto window = glfwCreateWindow(width, height, window_name.c_str(), NULL, NULL);

    if (window == NULL) 
    {
        std::fprintf(stderr, "GLFW: Failed to create a window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    std::fprintf(stderr, "GLFW: Window creation successful\n");

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return 1;
    }

    // std::jthread game_thread(game);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    int wait_ms = 3;
    
    std::vector<GLuint> shaders = 
    {
        add_shader("vert.glsl", GL_VERTEX_SHADER), 
        add_shader("frag.glsl", GL_FRAGMENT_SHADER)
    };

    GLuint program = create_program(shaders);
    std::vector<RenderData> render_data = initialise_render();

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) key_queue.try_enqueue(GLFW_KEY_W);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) key_queue.try_enqueue(GLFW_KEY_A);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) key_queue.try_enqueue(GLFW_KEY_S);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) key_queue.try_enqueue(GLFW_KEY_D);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) break;
        draw(render_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        glfwSwapBuffers(window);
    }
    return 0;
}
