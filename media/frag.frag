#version 330 core

in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye; // The inverse = position
} vsOut;

out vec4 fragColor;

uniform vec4 color;
uniform sampler2D baseTex;
uniform float shininess = 128.0;
uniform vec3 ambient = vec3(0.1);
uniform vec3 emissive = vec3(0.0);

uniform bool ambientPass;

const int LIGHT_TYPE_POINT = 0;
const int LIGHT_TYPE_DIR = 1;
const int LIGHT_TYPE_SPOT = 2;

struct LightParameters {
    int type;
    float range;
    vec3 color;
    vec3 position; // view/camera space
    vec3 direction; // view/camera space
};
uniform LightParameters light;

vec3 toLinear(in vec3 col) {
    return pow(col, vec3(2.2));
}

vec3 toGamma(in vec3 col) {
    return pow(col, vec3(1.0/2.2));
}

void main() {
    vec4 tex = texture2D(baseTex, vsOut.texCoord);
    tex.rgb = toLinear(tex.rgb);

    vec3 albedo =  color.rgb * tex.rgb;
    vec3 specColor = vec3(1.0, 1.0, 1.0);

    if(ambientPass) {
        fragColor = vec4(emissive + albedo * ambient, color.a * tex.a);
    } else {
        vec3 N = normalize(vsOut.normal); // renormalize because of interpolation?
        vec3 E = normalize(vsOut.eye);

        vec3 L;
        float L_atten;
        if(light.type == LIGHT_TYPE_DIR) {
            L = -light.direction;
            L_atten = 1.0;
        } else if(light.type == LIGHT_TYPE_POINT) {
            L = light.position + vsOut.eye;
            float dist = length(L);
            L = L / dist;
            L_atten = 1.0 - smoothstep(0.0, light.range, dist);
        }

        float rimLight = smoothstep(0.35, 1.0, 1.0 - max(dot(E, N), 0.0)) * 0.8;
        rimLight = 0.0;
        float lambert = max(dot(L, N), 0.0) + rimLight;
        lambert *= L_atten;

        vec3 reflectEye = normalize(reflect(-E, N));
        float specular = pow(max(dot(L, reflectEye), 0.0), shininess);
        specular *= L_atten;
        //specular = 0.0;

        fragColor = vec4((albedo * lambert + specular * specColor) * light.color, color.a * tex.a);
    }
}
