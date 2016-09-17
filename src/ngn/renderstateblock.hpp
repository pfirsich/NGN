#pragma once

#include <utility>

#include <glad/glad.h>

namespace ngn {
    class RenderStateBlock {
    public:
        enum class DepthFunc : GLenum {
            // If the depth test is disabled the depth buffer will not be written to.
            // If you want to write unconditionally (not test), use ALWAYS!
            DISABLED = 0,
            NEVER = GL_NEVER,
            LESS = GL_LESS,
            EQUAL = GL_EQUAL,
            LEQUAL = GL_LEQUAL,
            GREATER = GL_GREATER,
            NOTEQUAL = GL_NOTEQUAL,
            GEQUAL = GL_GEQUAL,
            ALWAYS = GL_ALWAYS
        };

        enum class FaceDirections : GLenum {
            NONE = 0,
            FRONT = GL_FRONT,
            BACK = GL_BACK,
            FRONT_AND_BACK
        };

        enum class FaceOrientation : GLenum {
            CW = GL_CW,
            CCW = GL_CCW
        };

        // https://www.opengl.org/wiki/Blending
        enum class BlendFactor : GLenum {
            ZERO = GL_ZERO,
            ONE = GL_ONE,
            SRC_COLOR = GL_SRC_COLOR,
            ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR,
            DST_COLOR = GL_DST_COLOR,
            ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR,
            SRC_ALPHA = GL_SRC_ALPHA,
            ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA,
            DST_ALPHA = GL_DST_ALPHA,
            ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA,
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

        // high level
        enum class BlendMode {
            REPLACE, // disable GL_BLEND
            ADD, // ONE, ONE and ADD
            MODULATE, // DST_COLOR, ZERO and ADD
            SCREEN, // ONE, ONE_MINUS_SRC_COLOR and ADD
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

        RenderStateBlock() : mDepthWrite(currentDepthWrite), mDepthFunc(currentDepthFunc), mCullFaces(currentCullFaces),
                             mFrontFace(currentFrontFace), mBlendEnabled(currentBlendEnabled), mBlendSrcFactor(currentBlendSrcFactor),
                             mBlendDstFactor(currentBlendDstFactor), mBlendEquation(currentBlendEquation) {}

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

        BlendEq getBlendEquation() const {return mBlendEquation;}
        void setBlendEquation(BlendEq eq) {mBlendEquation = eq;}

        void setBlendMode(BlendMode mode);

        void apply(bool force = false);

        // stencil func - glStencilFunc
        // stencil op - glStencilOp
    };
}