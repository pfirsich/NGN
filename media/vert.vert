// If you don't specify the version, it will assume OpenGL 1.1
#version 330 core

layout(location=0) in vec3 attrPosition;
layout(location=1) in vec3 attrNormal;
layout(location=2) in vec2 attrTexCoord;

out VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye;
} vsOut;

uniform mat4 modelview;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    vsOut.texCoord = attrTexCoord;
    vsOut.normal = normalize(normalMatrix * attrNormal);
    vsOut.eye = vec3(-modelview * vec4(attrPosition, 1.0));
    gl_Position = projection * modelview * vec4(attrPosition, 1.0);
}