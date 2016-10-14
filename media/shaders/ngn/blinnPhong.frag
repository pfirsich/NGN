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
        vec3 halfVector = normalize(eyeDir + lightDir);
        specular = pow(max(dot(surface.normal, halfVector), 0.0), surface.specularPower);
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

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

float rand3(in vec3 seed) {
    return fract(sin(dot(seed,vec3(53.1215, 21.1352, 9.1322))) * 2105.2354);
}

float rand2(in vec2 seed) {
    return fract(sin(dot(seed,vec2(12.9898,78.233))) * 43758.5453);
}

float shadowValue(in sampler2DShadow shadowMap, vec3 shadowCoords) {
    return texture(shadowMap, shadowCoords);
}

float poissonShadowValue(in sampler2DShadow shadowMap, vec3 shadowCoords) {
    // Should this parameter not be in texels, but in world coordinates?
    vec2 radius = 1.0/ngn_light.shadowMapSize * ngn_light.shadowPCFRadius; // it would be great to have this in world coordinates :/

    // Think about storing these rotations (sin(alpha) and cos(alpha)) in the RG channel of a texture and tile it over the screen
    // I could get rid of a sqrt, fract, sin, dot for a texture fetch. I don't know if this might be worth it.
    // Also I could store the whole poisson disk coordinate, if I don't plan on sorting my offsets for early bailing!
    // Storing this in a texture might be bad because we only fetch once into it and therefore effectively
    // trade two cache misses with some instructions. What I have now is good enough for now though.
    mat2 offsetRotation = mat2(1.0);
    // Interestingly using sin/cos seems slower than c = rand, s = sqrt(1-c*c), even though on newer hardware with a special functions unit
    // (like Kepler) that is idle mos of the time, sin/cos should be almost free. Seems like a reason to use a texture?
    // But it might be that it's slower, because here we use a lot of special functions anyways. Test this someday!

    // In theory using the world position as the variable for the rotation is the better way to do it, to avoid creeping noise
    // when the camera is moving, but in practice there is no visible difference. Don't forget this in case I decide to use a texture, because
    // if I would use the world position, I would need a 3D texture! (which I can then just not do)
    //float c = rand2(gl_FragCoord.xy);
    float c = rand3(vsOut.worldPos * 1000.0);
    float s = sqrt(1.0 - c*c);
    offsetRotation[0][0] = c;
    offsetRotation[1][1] = c;
    offsetRotation[0][1] = -s;
    offsetRotation[1][0] = s;

    float sum = 0.0;
    vec2 offset;

    for(int i = 0; i < ngn_light.shadowPCFEarlyBailSamples; ++i) {
        offset = poissonDisk[i + ngn_light.shadowPCFEarlyBailSamples] * radius;
        sum += texture(shadowMap, shadowCoords + vec3(offsetRotation * offset, 0.0));
    }

    float bailShadowFactor = sum / ngn_light.shadowPCFEarlyBailSamples;
    // I think this branching might be slow, especially with ngn_light.shadowPCFEarlyBailSamples == 0, I hope this somehow gets optimized out, even if
    // we only branch on a (homogeneous) uniform
    if(ngn_light.shadowPCFEarlyBailSamples == 0 || bailShadowFactor > 0.1 && bailShadowFactor < 0.9) {
        for(int i = 0; i < ngn_light.shadowPCFSamples - ngn_light.shadowPCFEarlyBailSamples; ++i) {
            offset = poissonDisk[i + ngn_light.shadowPCFEarlyBailSamples] * radius;
            sum += texture(shadowMap, shadowCoords + vec3(offsetRotation * offset, 0.0));
        }
        return sum / ngn_light.shadowPCFSamples;
    } else {
        return bailShadowFactor;
    }

}

vec4 colors[6] = vec4[6](
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(1.0, 0.0, 1.0, 1.0),
    vec4(0.0, 1.0, 1.0, 1.0)
);

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
        lightAtten = max(0, (lightAtten - cutoff) / (1.0 - cutoff));
    }

    if(ngn_light.shadowed) {
        int cascadeIndex = 0;
        for(int i = 0; i < ngn_light.shadowCascadeCount; ++i) {
            if(abs(vsOut.eye.z) < ngn_light.shadowCascadeSplitDistance[i]) {
                cascadeIndex = i;
                break;
            }
        }
        /*if(ngn_light.type == NGN_LIGHT_TYPE_DIRECTIONAL) {
            ngn_fragColor = colors[cascadeIndex]; return;
        }*/

        // normal offset: http://www.dissidentlogic.com/old/images/NormalOffsetShadows/GDC_Poster_NormalOffset.png
        // I need the minimal bias here, since on curved surfaces it sometimes happens, that the cosine does not rise fast enough
        // and I get rings of self-shadowing (especially with PCF!)
        float NdotL = dot(lightDir, vsOut.normal);
        float normalOffsetScale = min(1.0, sqrt(1.0 - NdotL*NdotL) + 0.5) * ngn_light.shadowNormalBias;
        //float normalOffsetScale = min(1.0, 1.0 -  + 0.0) * ngn_light.shadowNormalBias;
        vec3 normalOffset = vsOut.worldNormal * normalOffsetScale;
        vec4 offsetFragLightSpace = ngn_light.shadowMapCameraTransform[cascadeIndex] * vec4(vsOut.worldPos + normalOffset, 1.0);

        vec4 fragLightSpace = ngn_light.shadowMapCameraTransform[cascadeIndex] * vec4(vsOut.worldPos, 1.0);
        fragLightSpace.xy = offsetFragLightSpace.xy;
        vec3 shadowCoords = fragLightSpace.xyz / fragLightSpace.w;
        shadowCoords = shadowCoords * 0.5 + 0.5;
        shadowCoords.z -= ngn_light.shadowBias * 2.0;
        /*if(shadowCoords.x > 1.0 || shadowCoords.x < 0.0 || shadowCoords.y > 1.0 || shadowCoords.y < 0.0) {
            ngn_fragColor = vec4(0.0, 0.0, 1.0, 1.0); return;
        } else {
            //ngn_fragColor = vec4(vec3(pow(shadowCoords.z, 1000.0)), 1.0); return;
            ngn_fragColor = vec4(shadowCoords.xy, 0.0, 1.0); return;
        }*/

        shadowCoords.xy = (shadowCoords.xy + ngn_light.shadowMapUVOffset[cascadeIndex]) * ngn_light.shadowMapUVScale;

        float shadow = poissonShadowValue(ngn_light.shadowMap, shadowCoords);
        //ngn_fragColor = vec4(shadow); return;
        lightAtten *= shadow;
    }
}

#pragma ngn slot
void main() {
    SurfaceProperties _surf = surface();
    //Alpha test should be optional (different materials), so that it's occurence in the shader does not disable early-z
    //if(_surf.alpha < 1.0/256.0) discard; // alpha-test

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
