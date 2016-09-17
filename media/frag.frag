#version 330 core

in VSOUT {
    in vec2 texCoord;
    in vec3 normal;
    in vec3 eye; // The inverse = position
} vsOut;

out vec4 fragColor;

uniform vec3 lightDir; // view space
uniform vec3 color;

void main() {
    vec3 N = normalize(vsOut.normal); // renormalize because of interpolation?
    vec3 E = normalize(vsOut.eye);
    vec3 L = lightDir;
    float L_atten = 1.0; // directional

    vec3 albedo = color;
    vec3 specColor = vec3(1.0, 1.0, 1.0);
    vec3 ambiColor = vec3(1.0, 1.0, 1.0) * 0.1;
    float shininess = 128.0;

    float rimLight = smoothstep(0.35, 1.0, 1.0 - max(dot(E, N), 0.0)) * 0.8;
    rimLight = 0.0;
    float lambert = max(dot(L, N), 0.0) + rimLight;
    lambert *= L_atten;

    vec3 reflectEye = normalize(reflect(-E, N));
    float specular = pow(max(dot(L, reflectEye), 0.0), shininess);
    specular *= L_atten;
    //specular = 0.0;

    fragColor = vec4(albedo * (lambert + ambiColor) + specular * specColor, 1.0);
}
