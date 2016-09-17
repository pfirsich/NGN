#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ngn {
    const Texture* Texture::currentBoundTextures[Texture::MAX_UNITS] = {nullptr};

    void Texture::loadFromMemory(unsigned char* buffer, int len, int width, int height, int components, bool genMipmaps) {
        if(components < 1 || components > 4) {
            printf("Components must be >= 1 and <= 4\n");
            return;
        }
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
        loadFromMemory(buf, w*h*c*sizeof(unsigned char), w, h, c);
        delete[] buf;
        return true;
    }
}