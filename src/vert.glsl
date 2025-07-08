#version 430 core

// Input vertex data from the VBO
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Uniforms sent from the C++ application
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outputs to be passed to the fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    // Transform vertex position to world space and pass it to the fragment shader
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Transform normal vector to world space.
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Pass texture coordinates through to the fragment shader
    TexCoords = aTexCoords;
    
    // Calculate the final clip-space position of the vertex
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
