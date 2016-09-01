#version 330 core

in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye; // The inverse = position
    float brightness;
    vec3 localPos;
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

    vec3 rel = abs(vsOut.localPos - vec3(0.5));
    vec3 cutoff = (1.0 - smoothstep(0.499, 0.5, rel));
    const float smoothWidth = 0.005;
    const float edgeStart = 0.455;
    vec3 edges = smoothstep(edgeStart, edgeStart + smoothWidth, rel * cutoff);
    float edge = clamp(dot(edges, edges), 0.0, 1.0);
    vec3 col = vec3(edge) * mix(vec3(0.0, 0.66, 1.0), vec3(1.0), vsOut.brightness);
    //col = col * smoothstep(0.8, 1.0, dot(col, vec3(0.299, 0.587, 0.114)));
    color = vec4(col, 1.0); return;

    color = vec4(albedo * (lambert * diffColor + ambiColor) + specular * specColor, 1.0);
}
