#include <SDL.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ngn/ngn.hpp>

struct InputState {
    struct MouseState {
        int x, y;
        int lastX, lastY;
        int deltaX, deltaY;
        uint32_t buttons;

        MouseState() : x(-1), y(-1), lastX(-1), lastY(-1), deltaX(0), deltaY(0), buttons(0) {}
    } mouse;

    int numKeys;
    const uint8_t* key;

    InputState() : numKeys(0), key(nullptr) {}

    void update() {
        if(mouse.lastX < 0 and mouse.lastY < 0)
            SDL_GetMouseState(&mouse.lastX, &mouse.lastY);
        mouse.buttons = SDL_GetMouseState(&mouse.x, &mouse.y);
        mouse.deltaX = mouse.x - mouse.lastX; mouse.deltaY = mouse.y - mouse.lastY;
        mouse.lastX = mouse.x; mouse.lastY = mouse.y;

        if(key) delete key;

        key = SDL_GetKeyboardState(&numKeys);
    }
} inputState;

void moveCamera(ngn::Camera& camera, float dt) {
    float speed = 10.0 * dt;
    glm::vec3 move(0.0);
    move.x = ((int)inputState.key[SDL_SCANCODE_D] - (int)inputState.key[SDL_SCANCODE_A]);
    move.y = ((int)inputState.key[SDL_SCANCODE_R] - (int)inputState.key[SDL_SCANCODE_F]);
    move.z = ((int)inputState.key[SDL_SCANCODE_W] - (int)inputState.key[SDL_SCANCODE_S]);
    if(glm::length(move) > 0.5) {
        float y = move.y;
        move = camera.getLocalSystem() * move;
        move.y = y;
        move = speed * glm::normalize(move);
        camera.setPosition(camera.getPosition() + move);
    }

    if(inputState.mouse.buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        float sensitivity = 2.0f * 0.001f;
        camera.rotateWorld(sensitivity * inputState.mouse.deltaX, glm::vec3(0.0f, 1.0f, 0.0f));
        camera.rotate(sensitivity * inputState.mouse.deltaY, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}

int main(int argc, char** args) {
    ngn::setupDefaultLogging();

    ngn::Window window("ngn test", 1600, 900, false, false, 8);
    ngn::Renderer renderer;
    ngn::PerspectiveCamera camera(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);

    window.resizeSignal.connect([&camera, &renderer](int w, int h) {
        renderer.viewport = glm::ivec4(0, 0, w, h);
        camera.setAspect((float)w/h);
    });
    auto size = window.getSize();
    window.resizeSignal.emit(size.x, size.y);

    ngn::Resource::add("whitePixel", ngn::Texture::pixelTexture(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    /*ngn::Material baseMaterial(ngn::Resource::getPrepare<ngn::FragmentShader>("media/shaders/ngn/blinnPhong.frag"),
                               ngn::Resource::getPrepare<ngn::VertexShader>("media/shaders/ngn/default.vert"));
    baseMaterial.setTexture("baseTex", ngn::Resource::getPrepare<ngn::Texture>("whitePixel"));
    baseMaterial.setVector4("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    baseMaterial.setVector3("ambient", glm::vec3(0.1f));
    baseMaterial.setVector3("emissive", glm::vec3(0.0f));
    baseMaterial.setFloat("shininess", 512.0f);
    baseMaterial.addPass(ngn::Renderer::AMBIENT_PASS);
    baseMaterial.addPass(ngn::Renderer::LIGHT_PASS);*/
    ngn::ResourceHandle<ngn::Material> baseMaterial(ngn::Resource::getPrepare<ngn::Material>("media/materials/default.yml"));

    ngn::VertexFormat vFormat;
    vFormat.add(ngn::AttributeType::POSITION, 3, ngn::AttributeDataType::F32);
    vFormat.add(ngn::AttributeType::NORMAL, 3, ngn::AttributeDataType::F32);
    vFormat.add(ngn::AttributeType::TEXCOORD0, 2, ngn::AttributeDataType::F32);

    // Scene
    ngn::Scene scene;

    std::vector<ngn::Mesh*> meshes = ngn::assimpMeshes("media/ironman.obj", vFormat);
    ngn::Object ironman;
    for(auto mesh : meshes) {
        ngn::Object* obj = new ngn::Object;
        obj->setMesh(mesh);
        ironman.add(obj);
    }
    ironman.setMaterial(baseMaterial);
    ironman.setPosition(glm::vec3(20.0f, 0.0f, 0.0f));
    ironman.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    scene.add(&ironman);

    ngn::Object sphere;
    sphere.setMesh(ngn::sphereMesh(5.0f, 32, 32, vFormat));
    sphere.setMaterial(baseMaterial);
    sphere.setPosition(glm::vec3(-20.0f, 10.0f, 0.0f));
    scene.add(&sphere);

    ngn::Object ground;
    ground.setMesh(ngn::planeMesh(100.0f, 100.0f, 1, 1, vFormat));
    ground.setMaterial(new ngn::Material(*baseMaterial.getResource()));
    ground.getMaterial()->setVector4("color", glm::vec4(0.5f, 1.0f, 0.5f, 1.0f));
    ground.getMaterial()->setFloat("shininess", 64.0);
    scene.add(&ground);

    ngn::Object cube;
    cube.setMesh(ngn::boxMesh(10.0f, 10.0f, 10.0f, vFormat));
    cube.setMaterial(new ngn::Material(*baseMaterial.getResource()));
    cube.getMaterial()->setTexture("baseTex", ngn::Resource::getPrepare<ngn::Texture>("media/sq.png"));
    cube.getMaterial()->setVector4("color", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
    //cube.getMaterial()->setVector3("ambient", glm::vec3(1.0f));
    cube.getMaterial()->setBlendMode(ngn::Material::BlendMode::TRANSLUCENT);
    //cube.getMaterial()->setUnlit();
    //cube.getMaterial()->setDepthTest(ngn::DepthFunc::GREATER);
    cube.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
    scene.add(&cube);

    ngn::Light light;
    light.setLightData(new ngn::LightData, true);
    light.getLightData()->setType(ngn::LightData::LightType::DIRECTIONAL);
    light.getLightData()->setColor(glm::vec3(0.2f, 0.2f, 0.2f));
    light.lookAt(glm::vec3(-0.5f, -0.5f, -1.0f));
    scene.add(&light);

    ngn::Light pointLight;
    pointLight.setLightData(new ngn::LightData, true);
    pointLight.getLightData()->setType(ngn::LightData::LightType::POINT);
    pointLight.getLightData()->setRange(50.0f);
    pointLight.getLightData()->setColor(glm::vec3(1.0f, 0.25f, 0.25f));

    pointLight.setMesh(ngn::sphereMesh(1.0f, 10, 10, vFormat));
    pointLight.setMaterial(new ngn::Material(*baseMaterial.getResource()));
    pointLight.getMaterial()->setVector4("color", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    pointLight.getMaterial()->setVector3("emissive", pointLight.getLightData()->getColor());
    pointLight.getMaterial()->removePass(ngn::Renderer::LIGHT_PASS);
    //pointLight.getMaterial()->setUnlit();
    scene.add(&pointLight);

    camera.setPosition(glm::vec3(glm::vec3(0.0f, 5.0f, 50.0f)));

    LOG_DEBUG("blendenabled: %d", cube.getMaterial()->getStateBlock().getBlendEnabled());
    // Mainloop
    float lastTime = ngn::getTime();
    bool quit = false;
    window.closeSignal.connect([&quit]() {quit = true;});
    while(!quit) {
        float t = ngn::getTime();
        float dt = t - lastTime;
        lastTime = t;

        inputState.update();

        pointLight.setPosition(30.0f * glm::vec3(glm::cos(t), 0.0f, glm::sin(t)) + glm::vec3(0.0f, 10.0f, 0.0f));

        moveCamera(camera, dt);

        renderer.render(&scene, &camera, !inputState.key[SDL_SCANCODE_M], !inputState.key[SDL_SCANCODE_N]);

        window.updateAndSwap();
    }

    // Make a Screenshot when we close
    window.saveScreenshot("screenshot.png");

    return 0;
}