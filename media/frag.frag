#version 330 core

in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye; // The inverse = position
} vsOut;

out vec4 fragColor;

uniform vec4 color;
uniform sampler2D baseTex;
uniform float shininess = 256.0;
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

struct SurfaceProperties {
    vec3 albedo;
    vec3 normal;
    vec3 emission;
    float specularPower;
    float alpha;
};

SurfaceProperties surface() {
    SurfaceProperties ret;
    vec4 tex = texture2D(baseTex, vsOut.texCoord);
    tex.rgb = toLinear(tex.rgb);
    ret.albedo = color.rgb * tex.rgb;
    ret.alpha = color.a * tex.a;
    ret.normal = normalize(vsOut.normal); // renormalize because of interpolation?
    ret.emission = emissive;
    ret.specularPower = shininess;
    return ret;
}

vec4 lightingModel(in SurfaceProperties surface, in vec3 eyeDir, in vec3 lightDir, in float lightAtten) {
    float rimLight = smoothstep(0.35, 1.0, 1.0 - max(dot(eyeDir, surface.normal), 0.0)) * 0.8;
    rimLight = 0.0;
    float lambert = max(dot(lightDir, surface.normal), 0.0) + rimLight;

    //vec3 reflectEye = normalize(reflect(-eyeDir, surface.normal));
    //float specular = pow(max(dot(lightDir, reflectEye), 0.0), surface.specularPower);

    // blinn
    float specular = 0.0;
    if(dot(lightDir, surface.normal) > 0.0) {
        vec3 half = normalize(eyeDir + lightDir);
        specular = pow(max(dot(surface.normal, half), 0.0), shininess);
    }

    const vec3 specColor = vec3(1.0, 1.0, 1.0);
    return vec4((surface.albedo * lambert + specular * specColor) * light.color * lightAtten, surface.alpha);
}

void main() {
    SurfaceProperties _surf = surface();

    if(ambientPass) {
        fragColor = vec4(_surf.emission + _surf.albedo * ambient, _surf.alpha);
    } else {
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

        fragColor = lightingModel(_surf, E, L, L_atten);
    }
}
