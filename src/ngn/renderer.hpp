#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "scenenode.hpp"
#include "renderstateblock.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "uniformblock.hpp"
#include "rendererdata.hpp"

namespace ngn {
    // ForwardRenderer, DeferredRenderer
    // most classes should only contain data, while the renderer objects contain most of the rendering logic
    // not sure how materials will be decoupled from the renderers (if at all)

    class Renderer {
    protected:
        struct RenderQueueEntry {
            ShaderProgram* shaderProgram;
            std::vector<UniformBlock*> uniformBlocks;
            UniformList perEntryUniforms;
            Mesh* mesh;
            RenderStateBlock stateBlock;

            RenderQueueEntry(SceneNode* node) {
                Material* mat = node->getMaterial();
                assert(mat != nullptr); // Rendering a mesh without a material is impossible
                shaderProgram = mat->mShader;
                mesh = node->getMesh();
                stateBlock = mat->mStateBlock;
                uniformBlocks.push_back(mat);
            }
        };

        inline void renderRenderQueue(std::vector<RenderQueueEntry>& queue) {
            LOG_DEBUG("------- render\n");
            for(auto& entry : queue) {
                entry.stateBlock.apply();
                entry.shaderProgram->bind();
                for(auto block : entry.uniformBlocks) block->apply();
                entry.perEntryUniforms.apply();
                LOG_DEBUG("blend enabled: %d, factors: 0x%X, 0x%X\n", RenderStateBlock::currentBlendEnabled,
                    static_cast<int>(RenderStateBlock::currentBlendSrcFactor), static_cast<int>(RenderStateBlock::currentBlendDstFactor));
                entry.mesh->draw();
            }
        }

    private:
        int mRendererIndex;

    public:
        // Variables that store the current GL state
        static glm::vec4 currentClearColor;
        static float currentClearDepth;
        static GLint currentClearStencil;
        static glm::ivec4 currentViewport;
        static glm::ivec4 currentScissor;
        static bool currentScissorTest;

        static int nextRendererIndex;

        bool autoClear, autoClearColor, autoClearDepth, autoClearStencil;

        glm::vec4 clearColor;
        float clearDepth;
        GLint clearStencil;

        bool scissorTest;

        RenderStateBlock stateBlock;

        // x, y, width, height
        glm::ivec4 viewport;
        glm::ivec4 scissor;

        Renderer() : autoClear(true), autoClearColor(true), autoClearDepth(true), autoClearStencil(false),
                clearColor(currentClearColor), clearDepth(currentClearDepth), clearStencil(currentClearStencil), scissorTest(currentScissorTest),
                viewport(currentViewport), scissor(currentScissor) {
            stateBlock.setCullFaces(FaceDirections::BACK);
            stateBlock.setDepthTest(DepthFunc::LESS);
            stateBlock.apply(true);
            mRendererIndex = nextRendererIndex++;
            if(mRendererIndex >= SceneNode::MAX_RENDERDATA_COUNT)
                LOG_CRITICAL("More than SceneNode::MAX_RENDERDATA_COUNT(%d) renderers!", SceneNode::MAX_RENDERDATA_COUNT);
        }
        ~Renderer() {}

        // implement: single color, trilight (ground, sky, equator), cubemap
        //void setAmbientLightingModel(const LightingModel* model);

        void updateState() const;
        //setRenderTarget
        void clear(bool color, bool depth, bool stencil) const;
        void clear() const {clear(autoClearColor, autoClearDepth, autoClearStencil);}
        virtual void render(SceneNode* root, Camera* camera, bool regenerateQueue = true);
    };
}
