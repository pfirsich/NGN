#include <SDL.h>
#include <glad/glad.h>
#include <cstdio>
#include <memory>

const char* shaderSource = R"(
#version 330 core

#pragma x x x/x/x/x.x

out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
}
)";

int main(int argc, char** args) {
    // Window
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed! - '%s'\n", SDL_GetError()); return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int width = 1600, height = 900;
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    SDL_Window* mSDLWindow = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
    if(mSDLWindow == nullptr) {
        printf("SDL_CreateWindow failed! - '%s'\n", SDL_GetError()); return 1;
    }

    SDL_GLContext mSDLGLContext = SDL_GL_CreateContext(mSDLWindow);
    if(mSDLGLContext == 0) {
        printf("SDL_GL_CreateContext failed! - '%s'\n", SDL_GetError()); return 1;
    }

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Failed to initialize GLAD! - '%s'\n", SDL_GetError()); return 1;
    }

    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &shaderSource, nullptr);

    GLint compileStatus;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if(compileStatus == GL_FALSE){
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::unique_ptr<GLchar> shaderLog(new GLchar[logLength]);
        glGetShaderInfoLog(shader, logLength, nullptr, shaderLog.get());
        printf("shader compile failed: %s", shaderLog.get()); return 1;
    }
    printf("No one shall see me.\n");

    SDL_DestroyWindow(mSDLWindow);

    return 0;
}