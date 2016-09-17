#pragma once

#include <utility>

#include <glad/glad.h>

namespace ngn {
    class Texture {
    public:
        enum class WrapMode : GLenum {
            CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
            CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
            MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
            REPEAT = GL_REPEAT
        };

        // Excellent tutorial on texture filtering: https://paroj.github.io/gltut/Texturing/Tutorial%2015.html
        enum class MinFilter : GLenum {
            // I added the common names for these techniques, though they are not very good names
            NEAREST = GL_NEAREST,
            LINEAR = GL_LINEAR, // "Bilinear"
            NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
            LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST, // "Bilinear"
            NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR, // "Trilinear", but not "Bilinear" - you probably don't want to use this
            LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR // "Trilinear"
        };

        enum class MagFilter : GLenum {
            NEAREST = GL_NEAREST,
            LINEAR = GL_LINEAR
        };

    private:
        GLenum mTarget;
        GLuint mTextureObject;
        int mWidth, mHeight, mComponents;
        WrapMode mSWrap, mTWrap;
        MinFilter mMinFilter;
        MagFilter mMagFilter;

        void setParameter(GLenum param, GLenum val) {
            if(mTextureObject != 0) {
                bind();
                glTexParameteri(mTarget, param, val);
                unbind();
            }
        }

    public:
        static const size_t MAX_UNITS = 16;
        static const Texture* currentBoundTextures[MAX_UNITS];

        // Mipmapping is default, since it's takes a little more ram, but usually it's faster and looks nicer
        Texture(GLenum target = GL_TEXTURE_2D) : mTarget(target), mTextureObject(0),
                mSWrap(WrapMode::CLAMP_TO_EDGE), mTWrap(WrapMode::CLAMP_TO_EDGE), mMagFilter(MagFilter::LINEAR) {}
        Texture(const char* filename, bool genMipmaps = true, GLenum target = GL_TEXTURE_2D) : mTarget(target), mTextureObject(0),
                mSWrap(WrapMode::CLAMP_TO_EDGE), mTWrap(WrapMode::CLAMP_TO_EDGE), mMagFilter(MagFilter::LINEAR) {
            loadFromFile(filename, genMipmaps);
        }
        ~Texture() {
            glDeleteTextures(1, &mTextureObject);
        }

        void loadFromMemory(unsigned char* buffer, int len, int width, int height, int components, bool genMipmaps = true);
        bool loadEncodedFromMemory(unsigned char* encBuffer, int len, bool genMipmaps = true);
        bool loadFromFile(const char* filename, bool genMipmaps = true);

        void setTarget(GLenum target) {mTarget = target;}
        GLenum getTarget() const {return mTarget;}
        GLuint getTextureObject() const {return mTextureObject;}
        int getWidth() const {return mWidth;}
        int  getHeight() const {return mHeight;}

        void setWrap(WrapMode u, WrapMode v) {
            setParameter(GL_TEXTURE_WRAP_S, static_cast<GLenum>(mSWrap = u));
            setParameter(GL_TEXTURE_WRAP_T, static_cast<GLenum>(mTWrap = v));
        }
        std::pair<WrapMode, WrapMode> getWrap() const {return std::make_pair(mSWrap, mTWrap);}

        void setMinFilter(MinFilter filter) {setParameter(GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(mMinFilter = filter));}
        MinFilter getMinFilter() const {return mMinFilter;}
        void setMagFilter(MagFilter filter) {setParameter(GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(mMagFilter = filter));}
        MagFilter getMagFilter() const {return mMagFilter;}

        void bind(unsigned int unit = 0) const {
            if(currentBoundTextures[unit] != this) {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(mTarget, mTextureObject);
                currentBoundTextures[unit] = this;
            }
        }
        void unbind(unsigned int unit = 0) const {
            if(currentBoundTextures[unit] != nullptr) {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(mTarget, 0);
                currentBoundTextures[unit] = nullptr;
            }
        }
    };
}