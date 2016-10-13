#pragma once

#include <vector>

#include "rendertarget.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "shadercache.hpp"
#include "resource.hpp"
#include "renderstateblock.hpp"

namespace ngn {
    class PostEffectRender {
    private:
        static VertexShader* vertexShader;
        static Mesh* fullScreenMesh;
        static ShaderCache shaderCache;

        static bool staticInitialized;
        static void staticInitialize();

        ShaderProgram* mShaderProgram;
        bool mRendered;

    public:
        PostEffectRender(const ResourceHandle<FragmentShader>& shader) : mRendered(false) {
            if(!staticInitialized) staticInitialize();
            mShaderProgram = shaderCache.getShaderPermutation(0, shader.getResource(), vertexShader);
            if(mShaderProgram) mShaderProgram->bind();
            Texture::markAllUnitsAvailable();
        }

        ~PostEffectRender() {
            render();
        }

        void render() {
            if(!mRendered) {
                if(RenderStateBlock::currentDepthFunc != DepthFunc::DISABLED) {
                    RenderStateBlock::currentDepthFunc = DepthFunc::DISABLED;
                    glDisable(GL_DEPTH_TEST);
                }
                if(RenderStateBlock::currentBlendEnabled != false) {
                    RenderStateBlock::currentBlendEnabled = false;
                    glDisable(GL_BLEND);
                }
                fullScreenMesh->draw();
                mRendered = true;
            }
        }

        template<typename T>
        PostEffectRender& setUniform(const std::string& name, const T& val) {
            if(mShaderProgram) mShaderProgram->setUniform(name, val);
            return *this;
        }
    };

    class RendertargetHandle;
    class RendertargetPool {
    friend class RendertargetHandle;
    public:
        class RendertargetHandle {
        friend class RendertargetPool;
        private:
            Rendertarget& mRendertarget;
            RendertargetPool& mPool;
            int mRenderTargetIndex;

            RendertargetHandle(Rendertarget& rtarget, RendertargetPool& pool, int renderTargetIndex) :
                    mRendertarget(rtarget), mPool(pool), mRenderTargetIndex(renderTargetIndex) {}
        public:
            void release() {mPool.releaseRenderTarget(mRenderTargetIndex);}
            void bind() {mRendertarget.bind();}
            Rendertarget& get() {return mRendertarget;}
        };

    private:
        struct RenderTargetData {
            Rendertarget renderTarget;
            Texture renderTargetTexture;
            PixelFormat format;
            int width;
            int height;
            bool inUse;

            RenderTargetData(PixelFormat fmt, int w, int h) : format(fmt), width(w), height(h), inUse(false) {
                renderTargetTexture.setStorage(format, width, height);
                renderTarget.attachTexture(Rendertarget::Attachment::COLOR0, renderTargetTexture);
                //renderTarget.attachRenderbuffer(Rendertarget::Attachment::DEPTH, PixelFormat::DEPTH16, width, height);
            }
        };

        std::vector<RenderTargetData> mRendertargets;
        int mDefaultWidth, mDefaultHeight;

        void releaseRenderTarget(int index) {
            assert(static_cast<unsigned int>(index) < mRendertargets.size());
            mRendertargets[index].inUse = false;
        }

    public:
        RendertargetPool(int defaultWidth, int defaultHeight) : mDefaultWidth(defaultWidth), mDefaultHeight(defaultHeight) {}

        RendertargetHandle get(PixelFormat format, int width = -1, int height = -1) {
            if(width < 0) width = mDefaultWidth;
            if(height < 0) height = mDefaultHeight;

            for(size_t i = 0; i < mRendertargets.size(); ++i) {
                RenderTargetData& target = mRendertargets[i];
                if(target.format == format && target.width == width && target.height == height && !target.inUse) {
                    return RendertargetHandle(target.renderTarget, *this, i);
                }
            }
            // nothing available, add new to pool
            mRendertargets.emplace_back(format, width, height);
            RenderTargetData& target = mRendertargets.back();
            return RendertargetHandle(target.renderTarget, *this, mRendertargets.size() - 1);
        }

        RendertargetHandle bind(PixelFormat format, int width = -1, int height = -1) {
            RendertargetHandle ret = get(format, width, height);
            ret.bind();
            return ret;
        }
    };
}