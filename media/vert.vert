// If you don't specify the version, it will assume OpenGL 1.1
#version 330 core

layout(location=0) in vec3 attrPosition;
layout(location=1) in vec3 attrNormal;
layout(location=2) in vec2 attrTexCoord;

layout(location=12) in float attrScale;

out VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye;
    float brightness;
    vec3 localPos;
} vsOut;

uniform mat4 modelview;
uniform mat4 projection;
uniform mat3 normalMatrix;

uniform int gridSizeX;
uniform float scaleHeight;

void main() {
    vsOut.texCoord = attrTexCoord;
    vsOut.normal = normalize(normalMatrix * attrNormal);
    vec3 pos = attrPosition;
    int y = gl_InstanceID / gridSizeX;
    int x = gl_InstanceID % gridSizeX;
    pos.x += x;
    pos.y += y;
    pos.z *= attrScale;
    vsOut.brightness = attrScale / scaleHeight;
    vsOut.localPos = attrPosition;
    vsOut.eye = vec3(-modelview * vec4(pos, 1.0));
    gl_Position = projection * modelview * vec4(pos, 1.0);
}