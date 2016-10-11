// Includes can be anywhere in the program (their absolute position will not make a difference)
// Their order though will make huge difference!
#pragma ngn include media/shaders/ngn/gammaHelpers.glsl

in VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
    vec3 worldNormal;
    vec3 eye; // view space, inverse of position
} vsOut;

uniform vec4 color;
uniform sampler2D baseTex;
uniform float shininess;
uniform vec3 ambient;
uniform vec3 emissive;

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
    } else if(ngn_light.type == NGN_LIGHT_TYPE_POINT || ngn_light.type == NGN_LIGHT_TYPE_SPOT) {
        lightDir = ngn_light.position + vsOut.eye;
        float dist = length(lightDir);
        lightDir = lightDir / dist;

        lightAtten = dist / ngn_light.radius + 1.0;
        lightAtten = 1.0 / (lightAtten*lightAtten);

        if(ngn_light.type == NGN_LIGHT_TYPE_SPOT) {
            lightAtten *= 1.0 - smoothstep(ngn_light.innerAngle, ngn_light.outerAngle, dot(-lightDir, ngn_light.direction));
        }

        // attenCutoff represents only the cutoff of the attenuation function (or the cutoff of the actual RGB values outputted)
        // if we have light sources with luminance > 1, these values will obviously be wrong. therefore we have to rescale
        // also note, that we use max(r,g,b) as our luminance function, but just to make sure, that no component will exceed the cutoff
        float cutoff = ngn_light.attenCutoff / max(max(ngn_light.color.r, ngn_light.color.g), ngn_light.color.b);
        lightAtten = (lightAtten - cutoff) / (1.0 - cutoff);
    }

    if(ngn_light.shadowed) {
        // normal offset: http://www.dissidentlogic.com/old/images/NormalOffsetShadows/GDC_Poster_NormalOffset.png
        float normalOffsetScale = min(1.0, 1.0 - dot(lightDir, vsOut.normal)) * ngn_light.shadowNormalBias;
        vec3 normalOffset = vsOut.worldNormal * normalOffsetScale;
        vec4 offsetFragLightSpace = ngn_light.toLightSpace * vec4(vsOut.worldPos + normalOffset, 1.0);

        vec4 fragLightSpace = ngn_light.toLightSpace * vec4(vsOut.worldPos, 1.0);
        fragLightSpace.xy = offsetFragLightSpace.xy;
        vec3 shadowCoords = fragLightSpace.xyz / fragLightSpace.w;
        shadowCoords = shadowCoords * 0.5 + 0.5;
        /*if(shadowCoords.x > 1.0 || shadowCoords.x < 0.0 || shadowCoords.y > 1.0 || shadowCoords.y < 0.0) {
            ngn_fragColor = vec4(0.0, 0.0, 1.0, 1.0); return;
        } else {
            //ngn_fragColor = vec4(vec3(pow(shadowCoords.z, 1000.0)), 1.0); return;
            ngn_fragColor = vec4(shadowCoords.xy, 0.0, 1.0); return;
        }*/
        shadowCoords.z -= ngn_light.shadowBias;
        float shadow = texture(ngn_light.shadowMap, shadowCoords);
        //ngn_fragColor = vec4(shadow); return;
        lightAtten *= shadow;
    }
}

#pragma ngn slot
void main() {
    SurfaceProperties _surf = surface();
    if(_surf.alpha < 1.0/256.0) discard; // alpha-test

    #if NGN_PASS == NGN_PASS_FORWARD_AMBIENT
        ngn_fragColor = vec4(_surf.emission + _surf.albedo * ambient, _surf.alpha);
        //ngn_fragColor = vec4(0.0, 0.0, 0.1, 1.0);
    #elif NGN_PASS == NGN_PASS_FORWARD_LIGHT
        vec3 lightDir;
        float lightAtten;
        getLightDirAndAtten(lightDir, lightAtten); //return;

        vec3 E = normalize(vsOut.eye);
        ngn_fragColor = lightingModel(_surf, E, lightDir, lightAtten);
        ngn_fragColor.rgb = ngn_fragColor.rgb * ngn_light.color;
    #endif
}
