#pragma once

#include <glm/glm.hpp>
#include <SDL.h>
#include <glad/glad.h>
#include <SDL_opengl.h>

#include "signal.hpp"

namespace ngn {
    void checkGLError();

    // These values are only meaningful relative to each other
    float getTime();

    // If this can even be instantiated properly twice has never been tested.
    class Window {
    private:
        static bool firstCreation;

        SDL_Window* mSDLWindow;
        SDL_GLContext mSDLGLContext;

    public:
        static Window* currentWindow;

        using CloseSignalType = Signal<void()>; CloseSignalType closeSignal;
        using ResizeSignalType = Signal<void(int, int)>; ResizeSignalType resizeSignal;

        Window() : mSDLWindow(nullptr) {}
        Window(const char* title, int width, int height, bool fullscreen = false, bool vsync = true, int msaaSamples = 0, Uint32 createWindowFlags = 0) {
            create(title, width, height, fullscreen, vsync, msaaSamples, createWindowFlags);
        }
        ~Window() {SDL_DestroyWindow(mSDLWindow);}

        void create(const char* title, int width, int height, bool fullscreen = false, bool vsync = true, int msaaSamples = 0, Uint32 createWindowFlags = 0);

        void makeCurrent() const;
        void update();
        void swapBuffers() const;
        void updateAndSwap() {update(); swapBuffers();}
        glm::ivec2 getSize() {int w, h; SDL_GetWindowSize(mSDLWindow, &w, &h); return glm::ivec2(w, h);}
        SDL_Window* getHandle() const {return mSDLWindow;}
        SDL_GLContext getContext() const {return mSDLGLContext;}
        void saveScreenshot(const char* filename); // Saves the current contents of the frontbuffer to a png(!) file
    };
}
