out VSOUT {
    vec2 texCoord;
    vec3 normal;
    vec3 eye;
} vsOut;

void main() {
    vsOut.texCoord = attrTexCoord;
    vsOut.normal = normalize(normalMatrix * attrNormal);
    vsOut.eye = vec3(-modelview * vec4(attrPosition, 1.0));
    gl_Position = projection * modelview * vec4(attrPosition, 1.0);
}