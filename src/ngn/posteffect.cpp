#include "posteffect.hpp"

namespace ngn {
    VertexShader* PostEffectRender::vertexShader;
    Mesh* PostEffectRender::fullScreenMesh;
    ShaderCache PostEffectRender::shaderCache;

    bool PostEffectRender::staticInitialized = false;
    void PostEffectRender::staticInitialize() {
        staticInitialized = true;

        vertexShader = new VertexShader;
        vertexShader->setSource(R"(
out VSOUT {
    vec2 texCoord;
} vsOut;

layout(location = NGN_ATTR_POSITION) in vec2 attrPosition;

void main() {
    vsOut.texCoord = attrPosition * 0.5 + 0.5;
    gl_Position = vec4(attrPosition, 0.0, 1.0);
}
)");

        VertexFormat vFormat;
        vFormat.add(AttributeType::POSITION, 2, AttributeDataType::F32);
        fullScreenMesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        fullScreenMesh->addVertexBuffer(vFormat, 6);
        auto position = fullScreenMesh->getAccessor<glm::vec2>(AttributeType::POSITION);
        position[2] = glm::vec2(-1.0f,  1.0f);
        position[0] = glm::vec2(-1.0f, -1.0f);
        position[1] = glm::vec2( 1.0f,  1.0f);
        position[4] = glm::vec2( 1.0f,  1.0f);
        position[5] = glm::vec2(-1.0f, -1.0f);
        position[3] = glm::vec2( 1.0f, -1.0f);
        fullScreenMesh->hasAttribute(AttributeType::POSITION)->upload();
    }
}