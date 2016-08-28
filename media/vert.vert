// If you don't specify the version, it will assume OpenGL 1.1
#version 330 core

layout(location=0) in vec3 attrPosition;
layout(location=1) in vec2 attrTexCoord;
layout(location=2) in vec3 attrNormal;

out vec2 texCoord;
out vec3 normal;
out vec3 eye;

uniform mat4 modelview;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    texCoord = attrTexCoord;
    normal = normalize(normalMatrix * attrNormal);
    eye = vec3(-modelview * vec4(attrPosition, 1.0));
    gl_Position = projection * modelview * vec4(attrPosition, 1.0);
}