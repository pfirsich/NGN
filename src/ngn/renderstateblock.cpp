#include "renderstateblock.hpp"
#include "log.hpp"

namespace ngn {
    // These values are only accurate because we make sure that they are set when the program starts
    bool RenderStateBlock::currentDepthWrite = true;
    RenderStateBlock::DepthFunc RenderStateBlock::currentDepthFunc = RenderStateBlock::DepthFunc::LESS;
    RenderStateBlock::FaceDirections RenderStateBlock::currentCullFaces = RenderStateBlock::FaceDirections::BACK;
    RenderStateBlock::FaceOrientation RenderStateBlock::currentFrontFace = RenderStateBlock::FaceOrientation::CCW;
    bool RenderStateBlock::currentBlendEnabled = false;
    RenderStateBlock::BlendFactor RenderStateBlock::currentBlendSrcFactor = RenderStateBlock::BlendFactor::ONE;
    RenderStateBlock::BlendFactor RenderStateBlock::currentBlendDstFactor = RenderStateBlock::BlendFactor::ZERO;
    RenderStateBlock::BlendEq RenderStateBlock::currentBlendEquation = RenderStateBlock::BlendEq::ADD;

    void RenderStateBlock::setBlendMode(BlendMode mode) {
        switch(mode) {
            case BlendMode::REPLACE:
                mBlendEnabled = false;
                break;
            case BlendMode::ADD:
                mBlendEnabled = true;
                mBlendEquation = BlendEq::ADD;
                mBlendSrcFactor = BlendFactor::ONE;
                mBlendDstFactor = BlendFactor::ONE;
                break;
            case BlendMode::MODULATE:
                mBlendEnabled = true;
                mBlendEquation = BlendEq::ADD;
                mBlendSrcFactor = BlendFactor::DST_COLOR;
                mBlendDstFactor = BlendFactor::ZERO;
                break;
            case BlendMode::SCREEN:
                mBlendEnabled = true;
                mBlendEquation = BlendEq::ADD;
                mBlendSrcFactor = BlendFactor::ONE;
                mBlendDstFactor = BlendFactor::ONE_MINUS_SRC_COLOR;
                break;
        }
    }

    void RenderStateBlock::apply(bool force) const {
        if(mDepthWrite != currentDepthWrite || force) {
            glDepthMask(mDepthWrite);
            LOG_DEBUG("glDepthMask");
            currentDepthWrite = mDepthWrite;
        }

        if(mDepthFunc != currentDepthFunc || force) {
            if(mDepthFunc == DepthFunc::DISABLED) {
                glDisable(GL_DEPTH_TEST);
            } else {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(static_cast<GLenum>(mDepthFunc));
            }
            LOG_DEBUG("depth func");
            currentDepthFunc = mDepthFunc;
        }

        if(mCullFaces != currentCullFaces || force) {
            if(mCullFaces == FaceDirections::NONE) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(static_cast<GLenum>(mCullFaces));
            }
            currentCullFaces = mCullFaces;
            LOG_DEBUG("cull face");
        }

        if(mFrontFace != currentFrontFace || force) {
            glFrontFace(static_cast<GLenum>(mFrontFace));
            currentFrontFace = mFrontFace;
        }

        if(mBlendEnabled != currentBlendEnabled || force) {
            if(mBlendEnabled) {
                glEnable(GL_BLEND);
                if(mBlendSrcFactor != currentBlendSrcFactor || mBlendDstFactor != currentBlendDstFactor || force) {
                    glBlendFunc(static_cast<GLenum>(mBlendSrcFactor),
                                static_cast<GLenum>(mBlendDstFactor));
                    currentBlendSrcFactor = mBlendSrcFactor;
                    currentBlendDstFactor = mBlendDstFactor;
                    LOG_DEBUG("blend func");
                }
                if(mBlendEquation != currentBlendEquation || force) {
                    glBlendEquation(static_cast<GLenum>(mBlendEquation));
                    currentBlendEquation = mBlendEquation;
                    LOG_DEBUG("blend eq");
                }
            } else {
                glDisable(GL_BLEND);
            }
            LOG_DEBUG("blend enable");
            currentBlendEnabled = mBlendEnabled;
        }
    }
}