#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <cstdio>
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct RenderContext
{
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
		if(!glfwInit())
		{
			std::fprintf(stderr, "GLFW: Error initialising glfwInit\n");
			return false;
		}

		this->window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

		if(window == NULL)
		{
			std::fprintf(stderr, "GLFW: Failed to create a window\n");
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(this->window);
		std::fprintf(stderr, "GLFW: Window creation successful\n");

		if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return -1;
		}

		return true;
	}

	static std::string read_file(std::string file_path)
	{
		std::ifstream file(file_path.c_str());
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	void initialise_mvp()
	{
		// Projection matrix
		glm::mat4 projection =
			glm::perspective(glm::radians(45.0f), (float)1024 / (float)768, 0.1f, 100.0f);

		// Camera matrix
		glm::mat4 view = glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

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
		if(info_log_length > 0)
		{
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
		if(info_log_length > 0)
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

	void draw_triangle() { }
};

std::vector<std::string> split_string_by_delimiter(std::string delimiter, std::string input_string)
{
	std::string substring = "";
	std::vector<std::string> result;
	int i = 0;
	while(i < input_string.length())
	{
		std::string token = input_string.substr(i, delimiter.length());
		if(token == delimiter)
		{
			result.push_back(substring);
			substring = "";
			i += delimiter.size();
			continue;
		}
		substring += input_string[i];
		i += 1;
	}

	if(!substring.empty())
	{
		result.push_back(substring);
	}

	return result;
}

struct OBJ
{
	std::vector<float> vertices;
	std::vector<size_t> faces;

	void process_vertices(std::string line)
	{
		std::cout << line << std::endl;
		std::cout << "processing vertices" << std::endl;
	}

	void process_faces(std::string line)
	{
		std::cout << line << std::endl;
		std::cout << "processing faces" << std::endl;
	}

	void process_line(std::string line)
	{
		std::string identifier = line.substr(0, 2);
		if(identifier == "f ")
		{
			process_faces(line);
			return;
		}

		if(identifier == "v ")
		{
			process_vertices(line);
			return;
		}
	}

	void parse_obj(std::string file_path)
	{
		std::string obj_data = RenderContext::read_file(file_path);
		std::vector<std::string> splitted_obj = split_string_by_delimiter("\n", obj_data);
		for(std::string line : splitted_obj)
		{
			this->process_line(line);
		}
	}
};

int main()
{
	OBJ obj;
	obj.parse_obj("../assets/futurefemale/jkm_female1.obj");

	/*
	RenderContext render_context;
	if(!render_context.make_window(1024, 768, "Hello World"))
	{

		return -1;
	}

	// Load vertex shader
	std::string vertex_shader_code = render_context.read_file("../src/vertex.glsl");
	std::string fragment_shader_code = render_context.read_file("../src/fragment.glsl");
	render_context.load_shaders(vertex_shader_code, fragment_shader_code);

	render_context.initialise_mvp();

	static const GLfloat g_vertex_buffer_data[] = {
		-1.0f,
		-1.0f,
		0.0f,
		1.0f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
	};

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(
		GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	render_context.apply_mvp();
	while(!glfwWindowShouldClose(render_context.window))
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(0);
		render_context.update_screen();
	}

    */

	return 0;
}
