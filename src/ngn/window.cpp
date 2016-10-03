#include <chrono>

#include "window.hpp"
#include "log.hpp"

namespace ngn {
    bool Window::firstCreation = false;

    void checkGLError() {
        GLenum err = glGetError();
        if(err != GL_NO_ERROR) {
            std::string text("Unknown error");
            switch(err) {
                case GL_INVALID_ENUM:
                    text = "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE:
                    text = "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION:
                    text = "GL_INVALID_OPERATION"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    text = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                case GL_OUT_OF_MEMORY:
                    text = "GL_OUT_OF_MEMORY"; break;
            }
            LOG_WARNING("GL Error!: 0x%X - %s\n", err, text.c_str());
        }
    }

    float getTime() {
        static std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> diff = std::chrono::high_resolution_clock::now() - start;
        return diff.count();
    }

    void Window::create(const char* title, int width, int height, bool fullscreen, bool vsync, int msaaSamples, Uint32 createWindowFlags) {
        if(!firstCreation) {
            if(SDL_Init(SDL_INIT_VIDEO) < 0) {
                LOG_CRITICAL("SDL_Init failed! - '%s'\n", SDL_GetError());
                return;
            }
            firstCreation = true;
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

        LOG_DEBUG("samples: %d", msaaSamples);
        if(msaaSamples > 0) {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaSamples);
        }

        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | createWindowFlags | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        mSDLWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
        if(mSDLWindow == nullptr) {
            LOG_CRITICAL("SDL_CreateWindow failed! - '%s'\n", SDL_GetError());
            return;
        }

        mSDLGLContext = SDL_GL_CreateContext(mSDLWindow);
        if(mSDLGLContext == 0) {
            LOG_CRITICAL("SDL_GL_CreateContext failed! - '%s'\n", SDL_GetError());
            return;
        }

        if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            LOG_CRITICAL("Failed to initialize GLAD! - '%s'\n", SDL_GetError());
            return;
        }

        if(vsync) {
            if(SDL_GL_SetSwapInterval(1) < 0) {
                LOG_ERROR("SDL_GL_SetSwapInterval failed! - '%s'\n", SDL_GetError());
            }
        }

        glEnable(GL_FRAMEBUFFER_SRGB);
        if(msaaSamples > 0) glEnable(GL_MULTISAMPLE);
    }

    void Window::makeCurrent() const {
        if(SDL_GL_MakeCurrent(mSDLWindow, mSDLGLContext) < 0) {
            LOG_ERROR("SDL_GL_MakeCurrent failed! - '%s'\n", SDL_GetError());
            return;
        }
    }

    void Window::update() {
        SDL_Event e;
        while(SDL_PollEvent(&e) != 0) {
            switch(e.type) {
                case SDL_QUIT:
                    closeSignal.emit();
                    break;
                case SDL_WINDOWEVENT:
                    switch(e.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            resizeSignal.emit(e.window.data1, e.window.data2);
                            break;
                    }
                    break;
            }
        }
    }

    void Window::swapBuffers() const {
        SDL_GL_SwapWindow(mSDLWindow);
        #ifdef DEBUG
        // We do this right after the buffer swap, because glGetError might
        // trigger a flush, which will have happend in when swapping buffers anyway.
        checkGLError();
        #endif
    }
}