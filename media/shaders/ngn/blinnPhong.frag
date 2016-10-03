// Includes can be anywhere in the program (their absolute position will not make a difference)
// Their order though will make huge difference!
#pragma ngn include some other.glsl

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

// This is the way to do it in case someone wants to overwrite a slot partially and still wishes to use e.g. blinn-phong partly
vec4 blinnPhongLightingModel(in SurfaceProperties surface, in vec3 eyeDir, in vec3 lightDir, in float lightAtten) {
    float rimLight = smoothstep(0.35, 1.0, 1.0 - max(dot(eyeDir, surface.normal), 0.0)) * 0.8;
    rimLight = 0.0;
    float lambert = max(dot(lightDir, surface.normal), 0.0) + rimLight;

    // blinn
    float specular = 0.0;
    if(dot(lightDir, surface.normal) > 0.0) {
        vec3 half = normalize(eyeDir + lightDir);
        specular = pow(max(dot(surface.normal, half), 0.0), surface.specularPower);
    }

    const vec3 specColor = vec3(1.0, 1.0, 1.0);
    return vec4((surface.albedo * lambert + specular * specColor) * lightAtten, surface.alpha);
}

// I you wanted to provide an open slot (i.e. must-overwrite), you should use a forward declaration

#pragma ngn slot
vec4 lightingModel(in SurfaceProperties surface, in vec3 eyeDir, in vec3 lightDir, in float lightAtten) {
    return blinnPhongLightingModel(surface, eyeDir, lightDir, lightAtten);
}

SurfaceProperties blinnPhongSurface() {
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

#pragma ngn slot
SurfaceProperties surface() {
    return blinnPhongSurface();
}

void getLightDirAndAtten(out vec3 lightDir, out float lightAtten) {
    if(ngn_light.type == NGN_LIGHT_TYPE_DIRECTIONAL) {
        lightDir = -ngn_light.direction;
        lightAtten = 1.0;
    } else if(ngn_light.type == NGN_LIGHT_TYPE_POINT) {
        lightDir = ngn_light.position + vsOut.eye;
        float dist = length(lightDir);
        lightDir = lightDir / dist;
        lightAtten = 1.0 - smoothstep(0.0, ngn_light.range, dist);
    }
}

#pragma ngn slot
void main() {
    SurfaceProperties _surf = surface();

    #if NGN_PASS == NGN_PASS_FORWARD_AMBIENT
        ngn_fragColor = vec4(_surf.emission + _surf.albedo * ambient, _surf.alpha);
    #elif NGN_PASS == NGN_PASS_FORWARD_LIGHT
        vec3 lightDir;
        float lightAtten;
        getLightDirAndAtten(lightDir, lightAtten);

        vec3 E = normalize(vsOut.eye);
        ngn_fragColor = lightingModel(_surf, E, lightDir, lightAtten);
        ngn_fragColor.rgb = ngn_fragColor.rgb * ngn_light.color;
    #endif
}
