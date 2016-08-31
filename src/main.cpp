#include <SDL.h>
#include <glad/glad.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ngn/ngn.hpp>

ngn::Mesh* planeMesh;
ngn::Mesh* boxMesh;
ngn::VertexFormat vFormat, instanceDataFormat;

ngn::ShaderProgram *shader;
ngn::PerspectiveCamera camera(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
glm::vec4 light = glm::vec4(1.0, 0.5, 1.0, 1.0);

int gridSizeX = 100, gridSizeY = 100;

bool initGL() {
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    shader = new ngn::ShaderProgram();
    if(!shader->compileAndLinkFromFiles("media/frag.frag", "media/vert.vert")) {
        return false;
    }

    vFormat.add(ngn::AttributeType::POSITION, 3, ngn::AttributeDataType::F32);
    vFormat.add(ngn::AttributeType::NORMAL, 3, ngn::AttributeDataType::F32);

    instanceDataFormat.add(ngn::AttributeType::CUSTOM0, 1, ngn::AttributeDataType::F32, false, 1);

    planeMesh = ngn::planeMesh(1.0f, 1.0f, 1, 1, vFormat);
    planeMesh->normalize();
    planeMesh->transform(glm::scale(glm::mat4(), glm::vec3(1000.0f)));

    boxMesh = ngn::boxMesh(1.0f, 1.0f, 1.0f, vFormat);
    boxMesh->transform(glm::translate(glm::mat4(), glm::vec3(0.5f)));
    boxMesh->addVertexBuffer(instanceDataFormat, gridSizeX*gridSizeY, ngn::UsageHint::STREAM);

    camera.setPosition(glm::vec3(glm::vec3(0.0f, 0.0f, 100.0f)));

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

    float speed = 10.0 * dt;
    glm::vec3 move(0.0);
    move.x = ((int)keyState[SDL_SCANCODE_D] - (int)keyState[SDL_SCANCODE_A]);
    move.y = ((int)keyState[SDL_SCANCODE_R] - (int)keyState[SDL_SCANCODE_F]);
    move.z = ((int)keyState[SDL_SCANCODE_W] - (int)keyState[SDL_SCANCODE_S]);
    if(glm::length(move) > 0.5) {
        float y = move.y;
        move = camera.getLocalSystem() * move;
        move.y = y;
        move = speed * glm::normalize(move);
        camera.setPosition(camera.getPosition() + move);
    }

    if(buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        float sensitivity = 2.0f * 0.001f;
        camera.rotateWorld(sensitivity * deltaMouseX, glm::vec3(0.0f, 1.0f, 0.0f));
        camera.rotate(sensitivity * deltaMouseY, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}

void render(float dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto instanceData = boxMesh->getAccessor<float>(ngn::AttributeType::CUSTOM0);
    for(int y = 0; y < gridSizeY; ++y) {
        for(int x = 0; x < gridSizeX; ++x) {
            instanceData[y*gridSizeX+x] = (glm::cos(ngn::getTime() + (float)x / gridSizeX * M_PI * 3.0f) *
                                           glm::sin(ngn::getTime()*0.9f + (float)y / gridSizeY * M_PI * 4.0) + 1.0)
                                           * 0.5 * 5.0f;
        }
    }
    boxMesh->hasAttribute(ngn::AttributeType::CUSTOM0)->upload();

    glm::mat4 model;
    model = glm::translate(model, glm::vec3(-gridSizeX/2.0f, -gridSizeY/2.0f, 0.0f));

    glm::mat4 modelview = camera.getViewMatrix() * model;
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));

    light = light * glm::rotate(glm::mat4(), 1.0f * dt, glm::vec3(0.0f, 1.0, 0.0f));
    glm::vec3 lightDir = glm::normalize(glm::mat3(camera.getViewMatrix()) * glm::vec3(light));

    shader->bind();
    shader->setUniform("gridSizeX", gridSizeX);
    shader->setUniform("modelview", modelview);
    shader->setUniform("lightDir", lightDir);
    shader->setUniform("projection", camera.getProjectionMatrix());
    shader->setUniform("normalMatrix", normalMatrix);

    planeMesh->draw();
    boxMesh->draw(gridSizeX*gridSizeY);

    shader->unbind();
}

int main(int argc, char** args) {
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
        render(dt);

        window->updateAndSwap();
    }

    return 0;
}