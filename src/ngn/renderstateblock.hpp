#pragma once

#include <utility>

#include <glad/glad.h>

namespace ngn {
    enum class DepthFunc : GLenum {
        // If the depth test is disabled the depth buffer will not be written to.
        // If you want to write unconditionally (not test), use ALWAYS!
        DISABLED = 0,
        NEVER = GL_NEVER, // 0x0200
        LESS = GL_LESS, // 0x0201
        EQUAL = GL_EQUAL, // 0x0202
        LEQUAL = GL_LEQUAL, // 0x0203
        GREATER = GL_GREATER, // 0x0204
        NOTEQUAL = GL_NOTEQUAL, // 0x0205
        GEQUAL = GL_GEQUAL, // 0x0206
        ALWAYS = GL_ALWAYS // 0x0207
    };

    enum class FaceDirections : GLenum {
        NONE = 0,
        FRONT = GL_FRONT,
        BACK = GL_BACK,
        //FRONT_AND_BACK = GL_FRONT_AND_BACK
    };

    enum class FaceOrientation : GLenum {
        CW = GL_CW,
        CCW = GL_CCW
    };

    class RenderStateBlock {
    public:
        // https://www.opengl.org/wiki/Blending
        enum class BlendFactor : GLenum {
            ZERO = GL_ZERO, // 0
            ONE = GL_ONE, // 0
            SRC_COLOR = GL_SRC_COLOR, // 0x300
            ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR, // 0x301
            DST_COLOR = GL_DST_COLOR, // 0x306
            ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR, // 0x307
            SRC_ALPHA = GL_SRC_ALPHA, // 0x302
            ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA, // 0x303
            DST_ALPHA = GL_DST_ALPHA, // 0x304
            ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA, // 0x305
            CONSTANT_COLOR = GL_CONSTANT_COLOR,
            ONE_MINUS_CONSTANT_COLOR = GL_ONE_MINUS_CONSTANT_COLOR,
            CONSTANT_ALPHA = GL_CONSTANT_ALPHA,
            ONE_MINUS_CONSTANT_ALPHA = GL_ONE_MINUS_CONSTANT_ALPHA
        };

        enum class BlendEq : GLenum {
            ADD = GL_FUNC_ADD,
            SUBTRACT = GL_FUNC_SUBTRACT,
            REVERSE_SUBTRACT = GL_FUNC_REVERSE_SUBTRACT,
            MIN = GL_MIN,
            MAX = GL_MAX
        };

    private:
        bool mDepthWrite;
        DepthFunc mDepthFunc;
        FaceDirections mCullFaces;
        FaceOrientation mFrontFace;
        bool mBlendEnabled;
        BlendFactor mBlendSrcFactor, mBlendDstFactor;
        BlendEq mBlendEquation;

    public:
        static bool currentDepthWrite;
        static DepthFunc currentDepthFunc;
        static FaceDirections currentCullFaces;
        static FaceOrientation currentFrontFace;
        static bool currentBlendEnabled;
        static BlendFactor currentBlendSrcFactor;
        static BlendFactor currentBlendDstFactor;
        static BlendEq currentBlendEquation;

        RenderStateBlock() : mDepthWrite(true), mDepthFunc(DepthFunc::LESS), mCullFaces(FaceDirections::BACK),
                             mFrontFace(FaceOrientation::CCW), mBlendEnabled(false), mBlendSrcFactor(BlendFactor::ONE),
                             mBlendDstFactor(BlendFactor::ZERO), mBlendEquation(BlendEq::ADD) {}

        bool getDepthWrite() const {return mDepthWrite;}
        void setDepthWrite(bool write) {mDepthWrite = write;}

        DepthFunc getDepthTest() const {return mDepthFunc;}
        void setDepthTest(DepthFunc func = DepthFunc::LEQUAL) {mDepthFunc = func;}

        FaceDirections getCullFaces() const {return mCullFaces;}
        void setCullFaces(FaceDirections dirs = FaceDirections::BACK) {mCullFaces = dirs;}

        FaceOrientation getFrontFace() const {return mFrontFace;}
        void setFrontFace(FaceOrientation ori) {mFrontFace = ori;}

        // http://www.andersriggelsen.dk/glblendfunc.php
        bool getBlendEnabled() const {return mBlendEnabled;}
        void setBlendEnabled(bool blend) {mBlendEnabled = blend;}

        std::pair<BlendFactor, BlendFactor> getBlendFactors() const {return std::make_pair(mBlendSrcFactor, mBlendDstFactor);}
        void setBlendFactors(BlendFactor src, BlendFactor dst) {mBlendSrcFactor = src; mBlendDstFactor = dst;}
        void setBlendFactors(std::pair<BlendFactor, BlendFactor> factors) {setBlendFactors(factors.first, factors.second);}
        void setBlendSrcFactor(BlendFactor src) {mBlendSrcFactor = src;}
        void setBlendDstFactor(BlendFactor dst) {mBlendDstFactor = dst;}

        BlendEq getBlendEquation() const {return mBlendEquation;}
        void setBlendEquation(BlendEq eq) {mBlendEquation = eq;}

        void apply(bool force = false) const;

        // stencil func - glStencilFunc
        // stencil op - glStencilOp

        // this is intended for multiplass rendering - every additional pass added on top needs a different depth function
        // If depth write is enabled, this will be the depth function that will be used for all additional passes
        // mostly this means equal
        DepthFunc getAdditionalPassDepthFunc() const {
            if(mDepthWrite) {
                switch(mDepthFunc) {
                    case DepthFunc::DISABLED: return DepthFunc::DISABLED;
                    case DepthFunc::NEVER: return DepthFunc::NEVER; // this is not EQUAL, since the first time nothing has been written
                    case DepthFunc::LESS: return DepthFunc::EQUAL;
                    case DepthFunc::EQUAL: return DepthFunc::EQUAL;
                    case DepthFunc::LEQUAL: return DepthFunc::EQUAL;
                    case DepthFunc::GREATER: return DepthFunc::EQUAL;
                    case DepthFunc::NOTEQUAL: return DepthFunc::EQUAL;
                    case DepthFunc::GEQUAL: return DepthFunc::EQUAL;
                    case DepthFunc::ALWAYS: return DepthFunc::EQUAL;
                }
                return mDepthFunc; // This should in theory never be called, but the compilers whines if this is not here
            }
            return mDepthFunc;
        }
    };
}