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
ngn::Mesh* lightMesh;
ngn::Mesh* boxMesh;
ngn::VertexFormat vFormat, instanceDataFormat;

ngn::ShaderProgram *shader;
ngn::PerspectiveCamera camera(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
glm::vec4 light = glm::vec4(1.0, 0.5, 1.0, 1.0);
ngn::RenderStateBlock stateBlock;
ngn::UniformList uniformList;

glm::vec3 areaLightPos = glm::vec3(40.0f, 7.0f, 0.0f);
glm::vec2 areaLightSize = glm::vec2(20.0f, 10.0f);
glm::vec3 areaLightDir = glm::vec3(-1.0f, 0.0f, 0.0f);

int instances = 10;

bool initGL() {
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    stateBlock.setCullFaces(ngn::RenderStateBlock::FaceDirections::BACK);
    stateBlock.setDepthTest(ngn::RenderStateBlock::DepthFunc::LESS);

    shader = new ngn::ShaderProgram();
    if(!shader->compileAndLinkFromFiles("media/frag.frag", "media/vert.vert")) {
        return false;
    }

    vFormat.add(ngn::AttributeType::POSITION, 3, ngn::AttributeDataType::F32);
    vFormat.add(ngn::AttributeType::NORMAL, 3, ngn::AttributeDataType::F32);

    instanceDataFormat.add(ngn::AttributeType::CUSTOM0, 1, ngn::AttributeDataType::F32, false, 1);

    meshes = ngn::assimpMeshes("media/ironman.obj", vFormat);
    meshes.push_back(ngn::planeMesh(1000.0f, 1000.0f, 1, 1, vFormat));
    boxMesh = ngn::boxMesh(100.0f, 100.0f, 100.0f, vFormat);
    boxMesh->addVertexBuffer(instanceDataFormat, instances, ngn::UsageHint::DYNAMIC);

    lightMesh = ngn::planeMesh(1.0f, 1.0f, 1, 1, vFormat);
    lightMesh->transform(glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
                                   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                                   glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                   glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));

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
    for(int i = 0; i < instances; ++i) {
        instanceData[i] = glm::cos(ngn::getTime() + (float)i / instances * M_PI) * 150.0f;
    }
    boxMesh->hasAttribute(ngn::AttributeType::CUSTOM0)->upload();

    glm::mat4 model;
    model = glm::rotate(model, ngn::getTime() * 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    float scale = 0.1f;
    model = glm::scale(model, glm::vec3(scale, scale, scale));

    glm::mat4 modelview = camera.getViewMatrix() * model;
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));

    light = light * glm::rotate(glm::mat4(), 1.0f * dt, glm::vec3(0.0f, 1.0, 0.0f));
    glm::vec3 lightDir = glm::normalize(glm::mat3(camera.getViewMatrix()) * glm::vec3(light));

    areaLightDir = glm::vec3(-1.0f, -2.0 * (glm::sin(ngn::getTime()) * 0.5f + 0.5f), 0.0f);

    glm::mat3 areaLightBase;
    areaLightBase[2] = glm::normalize(areaLightDir);
    areaLightBase[0] = glm::normalize(glm::cross(areaLightBase[2], glm::vec3(0.0f, 1.0f, 0.0f)));
    areaLightBase[1] = glm::normalize(glm::cross(areaLightBase[0], areaLightBase[2]));

    uniformList.setMatrix4("projection", camera.getProjectionMatrix());
    uniformList.setMatrix3("normalMatrix", normalMatrix);

    uniformList.setVector3("lightDir", lightDir);

    // these are all given in camera space, since light calculations are easiest there (camera is at 0, 0, 0)
    uniformList.setVector3("areaLightPos", glm::vec3(camera.getViewMatrix() * glm::vec4(areaLightPos, 1.0f)));
    uniformList.setVector2("areaLightSize", areaLightSize);
    uniformList.setMatrix3("areaLightBase", glm::mat3(camera.getViewMatrix()) * areaLightBase);

    stateBlock.apply();
    shader->bind();
    uniformList.apply();

    shader->setUniform("modelview", modelview);
    shader->setUniform("ambient", 0.1f);
    for(auto mesh : meshes) mesh->draw();
    boxMesh->draw(instances);

    glm::mat4 lightModel = glm::translate(glm::mat4(), areaLightPos) *
                           glm::mat4(areaLightBase) *
                           glm::scale(glm::mat4(), glm::vec3(areaLightSize, 1.0f));
    shader->setUniform("modelview", camera.getViewMatrix() * lightModel);
    shader->setUniform("ambient", 1.0f);
    lightMesh->draw();

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