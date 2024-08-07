#version 330 core
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 MVP;
uniform mat3 ROT;
out vec2 TexCoord;

void main() {
    gl_Position = MVP * vec4(ROT * vertexPosition_modelspace, 1);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    // gl_Position = vec4(vertexPosition_modelspace, 1);
}
