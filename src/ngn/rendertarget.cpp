#include "rendertarget.hpp"

namespace ngn {
    void Rendertarget::prepare() {
        std::vector<GLenum> colorAttachments;
        auto isColor = [](Attachment attachment){return attachment != Attachment::DEPTH && attachment != Attachment::STENCIL && attachment != Attachment::DEPTH_STENCIL;};

        assert(mFBO == 0);
        glGenFramebuffers(1, &mFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        for(auto& texAttachment : mTextureAttachments) {
            Texture* tex = texAttachment.second.getResource();
            glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLenum>(texAttachment.first), tex->getTarget(),
                                   tex->getTextureObject(), 0);
            if(isColor(texAttachment.first)) colorAttachments.push_back(static_cast<GLenum>(texAttachment.first));
        }
        for(auto& renderBuffer : mRenderbufferAttachments) {
            glGenRenderbuffers(1, &renderBuffer.rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer.rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, renderBuffer.format, renderBuffer.width, renderBuffer.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, static_cast<GLenum>(renderBuffer.attachment), GL_RENDERBUFFER, renderBuffer.rbo);
            if(isColor(renderBuffer.attachment)) colorAttachments.push_back(static_cast<GLenum>(renderBuffer.attachment));
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        if(colorAttachments.size() == 0) { // depth/stencil only target
            glReadBuffer(GL_NONE);
            glDrawBuffer(GL_NONE);
        } else {
            glDrawBuffers(colorAttachments.size(), &colorAttachments[0]);
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Framebuffer object %d is incomplete after initialization!", mFBO);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    const Texture* Rendertarget::getTextureAttachment(Attachment attachment) const {
        for(auto& tex : mTextureAttachments) {
            if(tex.first == attachment) {
                return tex.second.getResource();
            }
        }
        return nullptr;
    }

}