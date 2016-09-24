#version 330 core

in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye; // The inverse = position
    vec3 lightDir; // world space
} vsOut;

out vec4 fragColor;

uniform vec3 color;
uniform sampler2D baseTex;
uniform float shininess = 128.0;

void main() {
    vec3 N = normalize(vsOut.normal); // renormalize because of interpolation?
    vec3 E = normalize(vsOut.eye);
    vec3 L = vsOut.lightDir;
    float L_atten = 1.0; // directional

    vec3 albedo = color * texture2D(baseTex, vsOut.texCoord).rgb;
    vec3 specColor = vec3(1.0, 1.0, 1.0);
    vec3 ambiColor = vec3(1.0, 1.0, 1.0) * 0.1;

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
