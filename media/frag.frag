#version 330 core

in VSOUT {
    in vec2 texCoord;
    in vec3 normal;
    in vec3 eye; // The inverse = position
} vsOut;

out vec4 color;

uniform vec3 lightDir; // view space

uniform vec3 areaLightPos;
uniform vec2 areaLightSize;
uniform mat3 areaLightBase;
uniform float ambient = 0.0;

vec3 boxIn(vec3 P) {
    vec3 inLightBase = transpose(areaLightBase) * (P - areaLightPos); // fragment position in light space
    inLightBase.xy = clamp(inLightBase.xy, -areaLightSize/2.0, areaLightSize/2.0);
    inLightBase.z = 0; // zero component along normal/area light direction
    return areaLightPos + areaLightBase * inLightBase;
}

void main() {
    vec3 N = normalize(vsOut.normal); // renormalize because of interpolation?
    vec3 E = normalize(vsOut.eye);
    vec3 L = lightDir;
    float L_atten = 1.0; // directional

    vec3 P = -vsOut.eye; // fragment position in camera space
    vec3 lightPos = boxIn(P);
    L = lightPos - P;
    float L_dist = length(L);
    L = normalize(L);
    float a = 0.00001;
    float b = 0.00001;
    L_atten = 1.0 / (1.0 + a * L_dist + b * L_dist*L_dist);
    //facing the plane -> brighter
    L_atten *= abs(dot(L, -areaLightBase[2]));
    if(isnan(L_atten)) L_atten = 1.0;
    //L_atten *= step(0.0, dot(-L, areaLightBase[2]));

    vec3 albedo = vec3(1.0, 1.0, 1.0);
    vec3 diffColor = vec3(1.0, 1.0, 1.0);
    vec3 specColor = vec3(1.0, 1.0, 1.0);
    vec3 ambiColor = vec3(1.0, 1.0, 1.0) * ambient;
    float shininess = 128.0;

    float rimLight = smoothstep(0.35, 1.0, 1.0 - max(dot(E, N), 0.0)) * 0.8;
    rimLight = 0.0;
    float lambert = max(dot(L, N), 0.0) + rimLight;
    lambert *= L_atten;

    vec3 reflectEye = normalize(reflect(-E, N));
    // project this reflected vector onto the plane (ray-plane intersection)
    // wiki: line_pos + line_dir * dot(plane_pos - line_pos, plane_normal) / dot(line_dir, plane_normal)
    vec3 projected = P + reflectEye * dot(areaLightPos - P, areaLightBase[2]) / dot(reflectEye, areaLightBase[2]);
    vec3 projectedDir = normalize(boxIn(projected) - P);
    float specular = pow(max(dot(reflectEye, projectedDir), 0.0), shininess);
    //float specular = pow(max(dot(-L, reflectEye), 0.0), shininess);
    specular *= L_atten;
    //specular = 0.0;

    color = vec4(albedo * (lambert * diffColor + ambiColor) + specular * specColor, 1.0);
}
