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
            const ShaderProgram* shaderProgram;
            std::vector<UniformBlock*> uniformBlocks;
            UniformList perEntryUniforms;
            Mesh* mesh;
            RenderStateBlock stateBlock;

            inline RenderQueueEntry(Material* mat, Material::Pass* pass, Mesh* _mesh) {
                shaderProgram = pass->getShaderProgram();
                uniformBlocks.push_back(mat);
                mesh = _mesh;
                stateBlock = pass->getStateBlock();
            }
        };

        inline void renderRenderQueue(std::vector<RenderQueueEntry>& queue) {
            //LOG_DEBUG("------- render");
            for(auto& entry : queue) {
                entry.stateBlock.apply();
                if(entry.shaderProgram) entry.shaderProgram->bind();
                for(auto block : entry.uniformBlocks) block->apply();
                entry.perEntryUniforms.apply();
                //LOG_DEBUG("blend enabled: %d, factors: 0x%X, 0x%X, depth write: %d, depth func: 0x%X", RenderStateBlock::currentBlendEnabled,
                //    static_cast<int>(RenderStateBlock::currentBlendSrcFactor), static_cast<int>(RenderStateBlock::currentBlendDstFactor),
                //    RenderStateBlock::currentDepthWrite, static_cast<int>(RenderStateBlock::currentDepthFunc));
                entry.mesh->draw();
            }
        }

    private:
        int mRendererIndex;

        static bool staticInitialized;
        static void staticInitialize();

    public:
        // Variables that store the current GL state
        static glm::vec4 currentClearColor;
        static float currentClearDepth;
        static GLint currentClearStencil;
        static glm::ivec4 currentViewport;
        static glm::ivec4 currentScissor;
        static bool currentScissorTest;

        static int nextRendererIndex;

        // If other renderers start defining these, they have to take care of not clashing with others themselves
        // Also it helps if their values are consecutive, so the staticInitialize-method can also easily implement a renderer query define
        static const int AMBIENT_PASS = 0;
        static const int LIGHT_PASS = 1;

        bool autoClear, autoClearColor, autoClearDepth, autoClearStencil;

        glm::vec4 clearColor;
        float clearDepth;
        GLint clearStencil;

        bool scissorTest;

        // x, y, width, height
        glm::ivec4 viewport;
        glm::ivec4 scissor;

        Renderer() : autoClear(true), autoClearColor(true), autoClearDepth(true), autoClearStencil(false),
                clearColor(currentClearColor), clearDepth(currentClearDepth), clearStencil(currentClearStencil), scissorTest(currentScissorTest),
                viewport(currentViewport), scissor(currentScissor) {
            if(!staticInitialized) staticInitialize();
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
