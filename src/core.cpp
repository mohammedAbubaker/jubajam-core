#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <iostream>

class RenderContext
{
	int x;

public:
	void print_render_context(std::string my_arg)
	{
		std::cout << "Hello render_context" << my_arg << std::endl;
	}
};
