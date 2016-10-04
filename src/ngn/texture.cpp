#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ngn {
    const Texture* Texture::currentBoundTextures[Texture::MAX_UNITS] = {nullptr};
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

    void Texture::loadFromMemory(unsigned char* buffer, int width, int height, int components, bool genMipmaps) {
        assert(components >= 1 && components <= 4);

        glGenTextures(1, &mTextureObject);
        glBindTexture(mTarget, mTextureObject);
        GLint formatMap[4] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
        GLint format = formatMap[components-1];
        glTexImage2D(mTarget, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
        if(genMipmaps) glGenerateMipmap(mTarget);
        glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, static_cast<GLenum>(mSWrap));
        glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, static_cast<GLenum>(mTWrap));
        glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(mMagFilter));
        glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(mMinFilter = genMipmaps ? MinFilter::LINEAR_MIPMAP_LINEAR : MinFilter::LINEAR));
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
}