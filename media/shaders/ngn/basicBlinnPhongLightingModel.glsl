#pragma ngn slot:lightingModel
vec4 lightingModel(in SurfaceProperties surface, in vec3 eyeDir, in vec3 lightDir, in float lightAtten) {
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
    return vec4((surface.albedo * lambert + specular * specColor) * light.color * lightAtten, surface.alpha);
}