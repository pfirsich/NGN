out VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye;
} vsOut;

void main() {
    vsOut.texCoord = attrTexCoord;
    vsOut.normal = normalize(ngn_normalMatrix * attrNormal);
    vsOut.eye = vec3(-ngn_modelViewMatrix * vec4(attrPosition, 1.0));
    gl_Position = ngn_modelViewProjectionMatrix * vec4(attrPosition, 1.0);
}