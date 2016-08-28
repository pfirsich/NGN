#include <SDL.h>
#include <glad/glad.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ngn/ngn.hpp>

std::vector<ngn::Mesh*> meshes;
ngn::Mesh* boxMesh;
ngn::VertexFormat vFormat;

ngn::ShaderProgram *shader;
glm::mat4 projectionMatrix;
ngn::PerspectiveCamera camera(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
glm::vec4 light = glm::vec4(1.0, 0.5, 1.0, 1.0);

bool initGL() {
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    shader = new ngn::ShaderProgram();
    if(!shader->compileAndLinkFromFiles("media/frag.frag", "media/vert.vert")) {
        return false;
    }

    vFormat.add("attrPosition", ngn::AttributeDataType::F32, 3, false, ngn::AttributeType::POSITION);
    vFormat.add("attrTexCoord", ngn::AttributeDataType::F32, 2, false, ngn::AttributeType::TEXCOORD);
    vFormat.add("attrNormal", ngn::AttributeDataType::F32, 3, false, ngn::AttributeType::NORMAL);

    //boxMesh = ngn::boxMesh(0.5f, 0.5f, 0.5f, vFormat);
    //boxMesh = ngn::sphereMesh(0.5f, 40, 40, vFormat);
    meshes = ngn::assimpMeshes("media/ironman.obj", vFormat);

    camera.setPosition(glm::vec3(glm::vec3(0.0f, 0.0f, 3.0f)));

    return true;
}


void moveCamera(float dt) {
    static int lastMouseX = -1, lastMouseY = -1;
    if(lastMouseX < 0 and lastMouseY < 0)
        SDL_GetMouseState(&lastMouseX, &lastMouseY);

    int x = 0, y = 0;
    Uint32 buttons = SDL_GetMouseState(&x, &y);
    int deltaMouseX = x - lastMouseX, deltaMouseY = y - lastMouseY;
    lastMouseX = x; lastMouseY = y;

    int numkeys = 0;
    const Uint8 *keyState = SDL_GetKeyboardState(&numkeys);

    float speed = 2.0 * dt;
    glm::vec3 move(0.0);
    move.x = ((int)keyState[SDL_SCANCODE_D] - (int)keyState[SDL_SCANCODE_A]);
    move.y = ((int)keyState[SDL_SCANCODE_R] - (int)keyState[SDL_SCANCODE_F]);
    move.z = ((int)keyState[SDL_SCANCODE_W] - (int)keyState[SDL_SCANCODE_S]);
    if(glm::length(move) > 0.5) {
        move = speed * glm::normalize(move);
        float y = move.y;
        move = camera.getLocalSystem() * move;
        move.y = y;
        move = glm::normalize(move);
        camera.setPosition(camera.getPosition() + move);
    }

    if(buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        float sensitivity = 2.0f * 0.001f;
        camera.rotateWorld(sensitivity * deltaMouseX, glm::vec3(0.0f, 1.0f, 0.0f));
        camera.rotate(sensitivity * deltaMouseY, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model;
    model = glm::rotate(model, ngn::getTime() * 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    float scale = 0.1f;
    model = glm::scale(model, glm::vec3(scale, scale, scale));

    glm::mat4 modelview = camera.getViewMatrix() * model;
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));

    light = light * glm::rotate(glm::mat4(), 0.007f, glm::vec3(0.0f, 1.0, 0.0f));
    glm::vec3 lightDir = glm::normalize(glm::mat3(camera.getViewMatrix()) * glm::vec3(light));

    shader->bind();
    shader->setUniform("modelview", modelview);
    shader->setUniform("projection", camera.getProjectionMatrix());
    shader->setUniform("normalMatrix", normalMatrix);
    shader->setUniform("lightDir", lightDir);
    for(auto mesh : meshes) mesh->draw();
    shader->unbind();
}

int main(int argc, char* args[])
{
    ngn::setupDefaultLogging();

    std::unique_ptr<ngn::Window> window(new ngn::Window("ngn test", 1600, 900));
    auto size = window->getSize();
    if(!initGL()) return 1;
    window->resizeSignal.connect([&](int w, int h) {
        glViewport(0, 0, w, h);
        camera.setAspect((float)w/h);
    });
    window->resizeSignal.emit(size.x, size.y);

    float lastTime = ngn::getTime();

    bool quit = false;
    window->closeSignal.connect([&]() {quit = true;});
    while(!quit) {
        float t = ngn::getTime();
        float dt = t - lastTime;
        lastTime = t;

        moveCamera(dt);
        render();

        window->updateAndSwap();
    }

    return 0;
}