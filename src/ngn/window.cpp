#include <chrono>
#include <cstring>
#include <unordered_map>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <SDL_log.h>

#include "window.hpp"
#include "log.hpp"

namespace ngn {
    bool Window::firstCreation = false;
    Window* Window::currentWindow = nullptr;

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

    std::unordered_map<GLenum, const char*> debugSourceName = {
        {GL_DEBUG_SOURCE_API, "api"},
        {GL_DEBUG_SOURCE_SHADER_COMPILER, "shader_compiler"},
        {GL_DEBUG_SOURCE_WINDOW_SYSTEM, "window_system"},
        {GL_DEBUG_SOURCE_THIRD_PARTY, "third_party"},
        {GL_DEBUG_SOURCE_APPLICATION, "application"},
        {GL_DEBUG_SOURCE_OTHER, "other"}
    };

    std::unordered_map<GLenum, const char*> debugTypeName = {
        {GL_DEBUG_TYPE_ERROR,"error"},
        {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,"deprecated_behavior"},
        {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,"undefined_behavior"},
        {GL_DEBUG_TYPE_PERFORMANCE,"performance"},
        {GL_DEBUG_TYPE_PORTABILITY,"portability"},
        {GL_DEBUG_TYPE_OTHER,"other"},
        {GL_DEBUG_TYPE_MARKER,"marker"},
        {GL_DEBUG_TYPE_PUSH_GROUP,"push_group"},
        {GL_DEBUG_TYPE_POP_GROUP,"pop_group"}
    };

    std::unordered_map<GLenum, const char*> debugSeverityName = {
        {GL_DEBUG_SEVERITY_HIGH, "high"},
        {GL_DEBUG_SEVERITY_MEDIUM, "medium"},
        {GL_DEBUG_SEVERITY_LOW, "low"},
        {GL_DEBUG_SEVERITY_NOTIFICATION, "notification"}
    };

    void __stdcall debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
        LOG_DEBUG("GL Debug message - source: %s, type: %s, severity: %s, message: %s", debugSourceName[source], debugTypeName[type], debugSeverityName[severity], message);
    }

    void SDLLogFunction(void* userdata, int category, SDL_LogPriority priority, const char* message) {
        LOG_DEBUG("SDL log: %s", message);
    }

    void Window::create(const char* title, int width, int height, bool fullscreen, bool vsync, int msaaSamples, Uint32 createWindowFlags) {
        SDL_LogSetOutputFunction(SDLLogFunction, nullptr);
        SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
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

        int contextFlags = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
        #ifdef DEBUG
            contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
        #endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);

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

        currentWindow = this;

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

        #ifdef DEBUG
        if(GLAD_GL_KHR_debug) {
            LOG_DEBUG("KHR_debug supported. Turning on debug output.");
            glDebugMessageCallback(debugCallback, nullptr);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
        } else if (GLAD_GL_ARB_debug_output) {
            LOG_DEBUG("ARB_debug_output supported. Turning on debug output.");
            glDebugMessageCallbackARB(debugCallback, nullptr);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
        }
        #endif
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

    void Window::saveScreenshot(const char* filename) {
        glm::ivec2 size = getSize();
        uint8_t* buf = new uint8_t[size.x*size.y*3];
        glReadPixels(0, 0, size.x, size.y, GL_RGB, GL_UNSIGNED_BYTE, buf);

        //flip y for some reason
        int lineSize = size.x*3;
        uint8_t* tempLine = new uint8_t[lineSize];
        for(int i = 0; i < size.y/2; ++i) {
            uint8_t* upperLine = buf + i*lineSize;
            uint8_t* lowerLine = buf + (size.y - 1 - i) * lineSize;
            std::memcpy(tempLine, upperLine, lineSize);
            std::memcpy(upperLine, lowerLine, lineSize);
            std::memcpy(lowerLine, tempLine, lineSize);
        }
        delete[] tempLine;

        if(!stbi_write_png(filename, size.x, size.y, 3, buf, 0)) {
            LOG_ERROR("Could not write screenshot '%s'", filename);
        }
        delete[] buf;
    }
}