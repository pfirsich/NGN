#version 330 core

in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye; // The inverse = position
    float brightness;
} vsOut;

out vec4 color;

uniform vec3 lightDir; // view space

void main() {
    vec3 N = normalize(vsOut.normal); // renormalize because of interpolation?
    vec3 E = normalize(vsOut.eye);
    vec3 L = lightDir;
    float L_atten = 1.0; // directional

    vec3 albedo = vec3(1.0, 1.0, 1.0);
    vec3 diffColor = vec3(1.0, 1.0, 1.0);
    vec3 specColor = vec3(1.0, 1.0, 1.0);
    vec3 ambiColor = vec3(1.0, 1.0, 1.0);
    float shininess = 128.0;

    float rimLight = smoothstep(0.35, 1.0, 1.0 - max(dot(E, N), 0.0)) * 0.8;
    rimLight = 0.0;
    float lambert = max(dot(L, N), 0.0) + rimLight;
    lambert *= L_atten;

    vec3 reflectEye = normalize(reflect(-E, N));
    float specular = pow(max(dot(-L, reflectEye), 0.0), shininess);
    specular *= L_atten;
    //specular = 0.0;

    color = vec4(vec3(1.0) * vsOut.brightness, 1.0); return;

    color = vec4(albedo * (lambert * diffColor + ambiColor) + specular * specColor, 1.0);
}
