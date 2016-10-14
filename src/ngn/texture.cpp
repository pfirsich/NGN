#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ngn {
    const Texture* Texture::currentBoundTextures[Texture::MAX_UNITS] = {nullptr};
    bool Texture::currentTextureUnitAvailable[Texture::MAX_UNITS] = {true};
    Texture* Texture::fallback = nullptr;
    bool Texture::staticInitialized = false;

    Texture* Texture::fromFile(const char* filename, bool genMipmaps) {
        Texture* ret = new Texture;
        if(ret->loadFromFile(filename, genMipmaps)) {
            return ret;
        } else {
            delete ret;
            return nullptr;
        }
    }

    Texture* Texture::pixelTexture(const glm::vec4& col) {
        Texture* temp = new ngn::Texture;
        uint32_t data = static_cast<int>(col.r * 255) <<  0 | static_cast<int>(col.g * 255) <<  8 |
                        static_cast<int>(col.b * 255) << 16 | static_cast<int>(col.a * 255) << 24;
        temp->loadFromMemory(reinterpret_cast<unsigned char*>(&data), 1, 1, 4, false);
        return temp;
    }

    void Texture::staticInitialize() {
        staticInitialized = true;

        Texture* tex = new Texture;
        int res = 32;
        uint32_t* buf = new uint32_t[res*res];
        for(int i = 0; i < res*res; ++i) buf[i] = (i + i/res) % 2 == 0 ? 0xFFFF00FF : 0xFF000000;
        tex->loadFromMemory(reinterpret_cast<uint8_t*>(buf), res, res, 4, false);
        tex->setMagFilter(MagFilter::NEAREST);
        fallback = tex;
    }

    void Texture::initSampler() {
        glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, static_cast<GLenum>(mSWrap));
        glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, static_cast<GLenum>(mTWrap));
        glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(mMagFilter));
        glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(mMinFilter));
    }

    void Texture::loadFromMemory(unsigned char* buffer, int width, int height, int components, bool genMipmaps) {
        assert(components >= 1 && components <= 4);

        if(mTextureObject == 0) glGenTextures(1, &mTextureObject);
        bind(0);

        GLint formatMap[4] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
        GLint format = formatMap[components-1];
        if(mImmutable || (width == mWidth && height == mHeight)) {
            if(mImmutable && (width != mWidth || height != mHeight)) {
                LOG_ERROR("Loading texture of size %d, %d that does not fit into an immutable texture of size %d, %d", width, height, mWidth, mHeight);
                return;
            }
            glTexSubImage2D(mTarget, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);
        } else {
            glTexImage2D(mTarget, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
            if(genMipmaps) mMinFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
            initSampler();
        }

        if(genMipmaps) glGenerateMipmap(mTarget);

        mWidth = width;
        mHeight = height;
    }

    bool Texture::loadEncodedFromMemory(unsigned char* encBuffer, int len, bool genMipmaps) {
        int w, h, c;
        unsigned char* buf = stbi_load_from_memory(encBuffer, len, &w, &h, &c, 0);
        if(buf == 0) {
            printf("Image could not be loaded from memory.");
            return false;
        }
        loadFromMemory(buf, w*h*c*sizeof(unsigned char), w, h, c);
        delete[] buf;
        return true;
    }

    bool Texture::loadFromFile(const char* filename, bool genMipmaps) {
        int w, h, c;
        unsigned char* buf = stbi_load(filename, &w, &h, &c, 0);
        if(buf == 0) {
            printf("Image file '%s' could not be loaded.\n", filename);
            return false;
        }
        loadFromMemory(buf, w, h, c);
        delete[] buf;
        return true;
    }

    void Texture::setStorage(PixelFormat internalFormat, int width, int height, int levels) {
        if(mTextureObject == 0) {
            glGenTextures(1, &mTextureObject);
        } else {
            if(mImmutable) {
                LOG_ERROR("Trying to set storage for an immutable texture! (you probably called setStorage on this texture twice)");
                return;
            }
        }
        bind(0);
        //glTexStorage2D(mTarget, levels, internalFormat, width, height);
        mWidth = width;
        mHeight = height;
        mImmutable = true;

        /*
        These are all glTexImage2D errors for invalid format combinations:
        GL_INVALID_OPERATION is generated if type is one of GL_UNSIGNED_BYTE_3_3_2, GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_5_6_5_REV, or GL_UNSIGNED_INT_10F_11F_11F_REV, and format is not GL_RGB.
        GL_INVALID_OPERATION is generated if type is one of GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_INT_8_8_8_8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_UNSIGNED_INT_10_10_10_2, GL_UNSIGNED_INT_2_10_10_10_REV, or GL_UNSIGNED_INT_5_9_9_9_REV, and format is neither GL_RGBA nor GL_BGRA.
        GL_INVALID_OPERATION is generated if format is GL_DEPTH_COMPONENT and internalFormat is not GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, or GL_DEPTH_COMPONENT32F.
        GL_INVALID_OPERATION is generated if internalFormat is GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, or GL_DEPTH_COMPONENT32F, and format is not GL_DEPTH_COMPONENT.

        There are way more combinations that don't go well together, but they are not in the docs, so I hope they will never make problems.
        The first two can be eliminated by using GL_FLOAT for type (for example), the other two are essentially the same.
        Ergo I will just just GL_RGBA for format and if internalFormat is a depth format, I will use GL_DEPTH_COMPONENT
        */
        GLenum intFormat = static_cast<GLenum>(internalFormat);
        GLenum format = GL_RGBA;
        if(intFormat == GL_DEPTH_COMPONENT || intFormat == GL_DEPTH_COMPONENT16 ||
           intFormat == GL_DEPTH_COMPONENT24 || intFormat == GL_DEPTH_COMPONENT32F) {
            format = GL_DEPTH_COMPONENT;
        }
        for(int i = 0; i < levels; ++i) {
            glTexImage2D(mTarget, i, intFormat, width, height, 0, format, GL_FLOAT, nullptr);
            width = width / 2;
            if(width < 1) width = 1;
            height = height / 2;
            if(height < 1) height = 1;
        }
        initSampler();
    }

    void Texture::updateData(GLenum format, GLenum type, const void* data, int level, int width, int height, int x, int y) {
        if(width < 0) width = mWidth;
        if(height < 0) height = mHeight;
        if(mTextureObject == 0) {
            LOG_ERROR("Trying to update texture that is not initialized yet!");
            return;
        }
        bind(0);
        glTexSubImage2D(mTarget, level, x, y, width, height, format, type, data);
    }
}