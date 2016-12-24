#include "rendertarget.hpp"

namespace ngn {
    Rendertarget* Rendertarget::currentRendertargetDraw = nullptr;
    Rendertarget* Rendertarget::currentRendertargetRead = nullptr;

    void Rendertarget::prepare() {
        std::vector<GLenum> colorAttachments;
        auto isColor = [](Attachment attachment){return attachment != Attachment::DEPTH && attachment != Attachment::STENCIL && attachment != Attachment::DEPTH_STENCIL;};

        assert(mFBO == 0);
        glGenFramebuffers(1, &mFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        for(auto& texAttachment : mTextureAttachments) {
            Texture* tex = texAttachment.second;
            glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLenum>(texAttachment.first), tex->getTarget(), tex->getTextureObject(), 0);
            if(isColor(texAttachment.first)) colorAttachments.push_back(static_cast<GLenum>(texAttachment.first));
        }
        for(auto& renderBuffer : mRenderbufferAttachments) {
            glGenRenderbuffers(1, &renderBuffer.rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer.rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(renderBuffer.format), renderBuffer.width, renderBuffer.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, static_cast<GLenum>(renderBuffer.attachment), GL_RENDERBUFFER, renderBuffer.rbo);
            if(isColor(renderBuffer.attachment)) colorAttachments.push_back(static_cast<GLenum>(renderBuffer.attachment));
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        if(colorAttachments.size() == 0) { // depth/stencil only target
            LOG_DEBUG("No color for %d", mFBO);
            glReadBuffer(GL_NONE);
            glDrawBuffer(GL_NONE);
        } else {
            LOG_DEBUG("%d color for %d", colorAttachments.size(), mFBO);
            LOG_DEBUG("color 0: %d", GL_COLOR_ATTACHMENT0);
            for(unsigned i = 0; i < colorAttachments.size(); ++i) LOG_DEBUG("attachment: %d", colorAttachments[i]);
            glDrawBuffers(colorAttachments.size(), &colorAttachments[0]);
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE) {
            std::unordered_map<GLenum, std::string> framebufferStatus = {
                {GL_FRAMEBUFFER_UNDEFINED, "undefined"},
                {GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "incomplete_attachment"},
                {GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, "incomplete_missing_attachment"},
                {GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "incomplete_draw_buffer"},
                {GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER, "incomplete_read_buffer"},
                {GL_FRAMEBUFFER_UNSUPPORTED, "unsupported"},
                {GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "incomplete_multisample"},
                {GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "incomplete_multisample"},
                {GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, "incomplete_layer_targets"}
            };
            LOG_ERROR("Framebuffer object %d is incomplete after initialization!: %s", mFBO, framebufferStatus[status].c_str());
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    const Texture* Rendertarget::getTextureAttachment(Attachment attachment) const {
        for(auto& tex : mTextureAttachments) {
            if(tex.first == attachment) {
                return tex.second;
            }
        }
        return nullptr;
    }

}