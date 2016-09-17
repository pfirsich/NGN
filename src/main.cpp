#include <SDL.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ngn/ngn.hpp>

#include <stack>
struct RenderQueueEntry {
    ngn::ShaderProgram* shaderProgram;
    std::vector<ngn::UniformBlock*> uniformBlocks;
    ngn::Mesh* mesh;
    const ngn::RenderStateBlock* stateBlock;

    RenderQueueEntry(ngn::ShaderProgram* shader, ngn::Mesh* mesh, const ngn::RenderStateBlock* state) :
            shaderProgram(shader), mesh(mesh), stateBlock(state) {}
};

void renderRenderQueue(std::vector<RenderQueueEntry>& queue) {
    for(auto& entry : queue) {
        entry.stateBlock->apply();
        entry.shaderProgram->bind();
        for(auto block : entry.uniformBlocks) block->apply();
        entry.mesh->draw();
    }
}

void render(ngn::SceneNode* root, ngn::Camera& camera, const glm::vec3& light) {
    static std::vector<RenderQueueEntry> renderQueue;
    if(renderQueue.size() == 0) renderQueue.reserve(2048);
    renderQueue.clear();

    ngn::UniformList globalUniforms;
    globalUniforms.setMatrix4("projection", camera.getProjectionMatrix());
    glm::vec3 lightDir = glm::normalize(glm::mat3(camera.getViewMatrix()) * light);
    globalUniforms.setVector3("lightDir", lightDir);

    std::vector<ngn::UniformList*> perObjectUniformList;

    // Build queue
    std::stack<ngn::SceneNode*> traversalStack;
    traversalStack.push(root);
    while(!traversalStack.empty()) {
        ngn::SceneNode* top = traversalStack.top();
        traversalStack.pop();

        // handle current node
        ngn::Mesh* mesh = top->getMesh();
        if(mesh) {
            ngn::Material* mat = top->getMaterial();
            assert(mat != nullptr); // Rendering a mesh without a material is impossible

            ngn::UniformList* objUniforms = new ngn::UniformList;
            perObjectUniformList.push_back(objUniforms);
            glm::mat4 model = top->getWorldMatrix();
            glm::mat4 modelview = camera.getViewMatrix() * model;
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelview)));
            objUniforms->setMatrix4("model", model);
            objUniforms->setMatrix4("modelview", modelview);
            objUniforms->setMatrix3("normalMatrix", normalMatrix);
            objUniforms->setMatrix4("modelviewprojection", camera.getProjectionMatrix() * modelview);

            renderQueue.emplace_back(mat->mShader, mesh, &(mat->mStateBlock));
            renderQueue.back().uniformBlocks.push_back(mat);
            renderQueue.back().uniformBlocks.push_back(objUniforms);
            renderQueue.back().uniformBlocks.push_back(&globalUniforms);
        }

        // chilren
        for(auto child : top->getChildren()) {
            traversalStack.push(child);
        }
    }

    renderRenderQueue(renderQueue);

    // Can I do this differently? (i.e. better)
    for(auto ulist : perObjectUniformList) delete ulist;
}

void moveCamera(ngn::Camera& camera, float dt) {
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

int main(int argc, char** args) {
    ngn::setupDefaultLogging();

    std::unique_ptr<ngn::Window> window(new ngn::Window("ngn test", 1600, 900));
    ngn::PerspectiveCamera camera(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
    window->resizeSignal.connect([&camera](int w, int h) {
        glViewport(0, 0, w, h);
        camera.setAspect((float)w/h);
    });
    auto size = window->getSize();
    window->resizeSignal.emit(size.x, size.y);

    ngn::Texture* whitePixel = new ngn::Texture;
    uint32_t data = 0xFFFFFFFF;
    whitePixel->loadFromMemory(reinterpret_cast<unsigned char*>(&data), 4, 1, 1, 4, false);

    // Scene
    ngn::RenderStateBlock globalStateBlock;
    globalStateBlock.setCullFaces(ngn::RenderStateBlock::FaceDirections::BACK);
    globalStateBlock.setDepthTest(ngn::RenderStateBlock::DepthFunc::LESS);
    globalStateBlock.apply(true);

    ngn::ShaderProgram *shader = new ngn::ShaderProgram();
    if(!shader->compileAndLinkFromFiles("media/frag.frag", "media/vert.vert")) {
        return false;
    }

    ngn::VertexFormat vFormat;
    vFormat.add(ngn::AttributeType::POSITION, 3, ngn::AttributeDataType::F32);
    vFormat.add(ngn::AttributeType::NORMAL, 3, ngn::AttributeDataType::F32);
    vFormat.add(ngn::AttributeType::TEXCOORD0, 2, ngn::AttributeDataType::F32);

    ngn::Scene scene;

    std::vector<ngn::Mesh*> meshes = ngn::assimpMeshes("media/ironman.obj", vFormat);
    ngn::Object ironman;
    for(auto mesh : meshes) {
        ngn::Object* obj = new ngn::Object;
        obj->setMesh(mesh);
        ironman.add(obj);
    }
    ironman.setMaterial(new ngn::Material(shader));
    ironman.getMaterial()->setVector3("color", glm::vec3(1.0f, 1.0f, 1.0f));
    ironman.getMaterial()->setTexture("baseTex", whitePixel);
    ironman.getMaterial()->setFloat("shininess", 512.0f);
    ironman.setPosition(glm::vec3(20.0f, 0.0f, 0.0f));
    ironman.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    scene.add(&ironman);

    ngn::Object ground;
    ground.setMesh(ngn::planeMesh(100.0f, 100.0f, 1, 1, vFormat));
    ground.setMaterial(new ngn::Material(shader));
    ground.getMaterial()->setVector3("color", glm::vec3(0.5f, 1.0f, 0.5f));
    ground.getMaterial()->setTexture("baseTex", whitePixel);
    ground.getMaterial()->setFloat("shininess", 64.0);
    scene.add(&ground);

    ngn::Object cube;
    cube.setMesh(ngn::boxMesh(10.0f, 10.0f, 10.0f, vFormat));
    cube.setMaterial(new ngn::Material(shader));
    cube.getMaterial()->setTexture("baseTex", new ngn::Texture("media/sq.png"));
    cube.getMaterial()->setVector3("color", glm::vec3(1.0f, 1.0f, 1.0f));
    cube.getMaterial()->setFloat("shininess", 256.0);
    cube.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
    scene.add(&cube);

    camera.setPosition(glm::vec3(glm::vec3(0.0f, 0.0f, 3.0f)));

    // Mainloop
    float lastTime = ngn::getTime();
    bool quit = false;
    window->closeSignal.connect([&]() {quit = true;});
    while(!quit) {
        float t = ngn::getTime();
        float dt = t - lastTime;
        lastTime = t;

        moveCamera(camera, dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        globalStateBlock.apply();
        glm::vec3 lightDir = glm::vec3(1.0f, 0.3f, 1.0f);
        render(&scene, camera, lightDir);

        window->updateAndSwap();
    }

    return 0;
}