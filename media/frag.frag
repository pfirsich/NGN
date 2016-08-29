#version 330 core

in VSOUT {
    in vec2 texCoord;
    in vec3 normal;
    in vec3 eye;
} vsOut;

out vec4 color;

uniform vec3 lightDir; // view space

void main() {
    vec3 N = normalize(vsOut.normal); // renormalize because of interpolation?
    vec3 E = normalize(vsOut.eye);
    vec3 L = lightDir;
    vec3 reflectDir = reflect(-L, N);

    vec3 diffColor = vec3(1.0, 1.0, 1.0);
    vec3 specColor = vec3(1.0, 1.0, 1.0) * 1.0;
    vec3 ambiColor = vec3(1.0, 1.0, 1.0) * 0.05;
    float shininess = 256.0;

    float lambert = max(dot(L, N), 0.0) + smoothstep(0.35, 1.0, 1.0 - max(dot(E, N), 0.0)) * 0.8;
    float specular = pow(max(dot(E, reflectDir), 0.0), shininess);
    vec4 albedo = vec4(1.0, 1.0, 1.0, 1.0);
    color = vec4(albedo.rgb * (lambert * diffColor + ambiColor) + specular * specColor, 1.0);
}
