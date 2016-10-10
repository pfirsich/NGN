out VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
    vec3 eye;
} vsOut;

layout(location = NGN_ATTR_POSITION) in vec3 attrPosition;
layout(location = NGN_ATTR_NORMAL) in vec3 attrNormal;
layout(location = NGN_ATTR_TEXCOORD0) in vec2 attrTexCoord;

void main() {
    vsOut.texCoord = attrTexCoord;
    vsOut.normal = normalize(ngn_normalMatrix * attrNormal);
    vsOut.worldPos = vec3(ngn_modelMatrix * vec4(attrPosition, 1.0));
    vsOut.eye = vec3(-ngn_modelViewMatrix * vec4(attrPosition, 1.0));
    gl_Position = ngn_modelViewProjectionMatrix * vec4(attrPosition, 1.0);
}