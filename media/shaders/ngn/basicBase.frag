in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye; // The inverse = position
} vsOut;

/*********** PUT THIS AWAY ***********/
// for both frag and vert
vec3 toLinear(in vec3 col) {
    return pow(col, vec3(2.2));
}
/*************************************/

struct SurfaceProperties {
    vec3 albedo;
    vec3 normal;
    vec3 emission;
    float specularPower;
    float alpha;
};

out vec4 ngn_fragColor;

// you may forward declare open slots, or provide an empty body "{}".
#pragma ngn slot:surface
SurfaceProperties surface();

#pragma ngn slot:lightingModel
vec4 lightingModel(in SurfaceProperties surface, in vec3 eyeDir, in vec3 lightDir, in float lightAtten);

#pragma ngn slot:frag
void main() {
    SurfaceProperties _surf = surface();

    #if NGN_PASS == NGN_PASS_FORWARD_AMBIENT
        ngn_fragColor = vec4(_surf.emission + _surf.albedo * ambient, _surf.alpha);
    #elif NGN_PASS == NGN_PASS_FORWARD_LIGHT
        vec3 E = normalize(vsOut.eye);
        vec3 L;
        float L_atten;
        if(ngn_light.type == NGN_LIGHT_TYPE_DIRECTIONAL) {
            L = -ngn_light.direction;
            L_atten = 1.0;
        } else if(ngn_light.type == NGN_LIGHT_TYPE_POINT) {
            L = ngn_light.position + vsOut.eye;
            float dist = length(L);
            L = L / dist;
            L_atten = 1.0 - smoothstep(0.0, ngn_light.range, dist);
        }

        ngn_fragColor = lightingModel(_surf, E, L, L_atten);
        ngn_fragColor.rgb = ngn_fragColor.rgb * ngn_light.color;
    #endif
}
