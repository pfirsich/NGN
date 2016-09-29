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

#pragma ngn slot:surface
SurfaceProperties surface() {
    THIS WILL ALWAYS BE OVERWRITTEN
    I CAN WRITE ANYTHING I WANT IN HERE // }
    /* } */
}

// this is how you declare an empty slot! "{}"! only forward declaring it will not work (yet)
#pragma ngn slot:lightingModel
vec4 lightingModel(in SurfaceProperties surface, in vec3 eyeDir, in vec3 lightDir, in float lightAtten) {}

#pragma ngn slot:frag
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
