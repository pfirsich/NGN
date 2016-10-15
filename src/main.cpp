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
    float speed = 30.0f;
    if(inputState.key[SDL_SCANCODE_LSHIFT]) speed *= 2.0f;
    speed *= dt;

    glm::vec3 move(0.0f);
    move.x = ((int)inputState.key[SDL_SCANCODE_D] - (int)inputState.key[SDL_SCANCODE_A]);
    move.y = ((int)inputState.key[SDL_SCANCODE_R] - (int)inputState.key[SDL_SCANCODE_F]);
    move.z = ((int)inputState.key[SDL_SCANCODE_W] - (int)inputState.key[SDL_SCANCODE_S]);
    if(glm::length(move) > 0.5f) {
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

    ngn::Window window("ngn test", 1600, 900, false, false);

    ngn::Renderer renderer;
    renderer.clearColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

    ngn::PerspectiveCamera camera(glm::radians(45.0f), 1.0f, 2.0f, 400.0f);
    //ngn::OrthographicCamera camera(-50.0f, 50.0f, -50.0f, 50.0f, 0.0f, 200.0f);

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

    std::vector<std::pair<std::string, ngn::Mesh*> > meshes = ngn::assimpMeshes("media/ironman.obj", true, vFormat);
    ngn::Object ironman;
    for(auto& mesh : meshes) {
        ngn::Object* obj = new ngn::Object;
        obj->setMesh(mesh.second);
        ironman.add(*obj);
    }
    ironman.setMaterial(baseMaterial);
    ironman.setPosition(glm::vec3(20.0f, 0.0f, 0.0f));
    ironman.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    scene.add(ironman);

    std::vector<std::pair<std::string, ngn::Mesh*> > testSceneMeshes = ngn::assimpMeshes("media/ngn_testscene.obj", false, vFormat);
    ngn::Object testScene;
    LOG_DEBUG("%d meshes in test scene", testSceneMeshes.size());
    for(auto& mesh : testSceneMeshes) {
        ngn::Object* obj = new ngn::Object;
        obj->setMesh(mesh.second);
        testScene.add(*obj);
        //LOG_DEBUG("name: %s", mesh.first.c_str());
    }
    testScene.setMaterial(baseMaterial);
    testScene.setScale(2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    //testScene.rotate(M_PI/2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    scene.add(testScene);

    ngn::Object sphere;
    sphere.setMesh(ngn::sphereMesh(5.0f, 128, 128, true, vFormat));
    sphere.setMaterial(baseMaterial);
    sphere.setPosition(glm::vec3(-20.0f, 10.0f, 0.0f));
    scene.add(sphere);

    /*ngn::Object ground;
    ground.setMesh(ngn::planeMesh(100.0f, 100.0f, 1, 1, vFormat));
    ground.setMaterial(new ngn::Material(*baseMaterial.getResource()));
    ground.getMaterial()->setVector4("color", glm::vec4(0.5f, 1.0f, 0.5f, 1.0f));
    ground.getMaterial()->setFloat("shininess", 64.0);
    scene.add(ground);*/

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
    scene.add(cube);

    ngn::Light dirLight;
    dirLight.addLightData(ngn::LightData::LightType::DIRECTIONAL);
    dirLight.getLightData()->setColor(1.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    dirLight.getLightData()->addShadow(4096, 4096, 4);
    dirLight.lookAt(glm::vec3(-0.4f, -1.0f, -0.4f));
    //dirLight.getLightData()->getShadow()->getCamera()->addDebugMesh();
    scene.add(dirLight);

    ngn::Light spotLight;
    spotLight.addLightData(ngn::LightData::LightType::SPOT);
    spotLight.getLightData()->setColor(100.0f * glm::vec3(0.25f, 0.25f, 1.0f));
    spotLight.setPosition(glm::vec3(35.0f, 25.0f, -145.0f));
    spotLight.lookAt(spotLight.getPosition() + glm::vec3(0.0f, -1.0f, 0.5f));
    spotLight.getLightData()->addShadow(2048, 2048);
    //spotLight.getLightData()->getShadow()->getCamera()->lookAtPos(glm::vec3(30.0f, 30.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    //spotLight.getLightData()->getShadow()->getCamera()->addDebugMesh();
    scene.add(spotLight);

    ngn::Light pointLight;
    pointLight.addLightData(ngn::LightData::LightType::POINT);
    pointLight.getLightData()->setRadius(1.0f);
    glm::vec3 col(1.0f, 0.25f, 0.25f);
    pointLight.getLightData()->setColor(100.0f * col);

    pointLight.setMesh(ngn::sphereMesh(pointLight.getLightData()->getRadius(), 32, 32, false, vFormat));
    pointLight.setMaterial(new ngn::Material(*baseMaterial.getResource()));
    pointLight.getMaterial()->setVector4("color", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    pointLight.getMaterial()->setVector3("emissive", col);
    pointLight.getMaterial()->removePass(ngn::Renderer::LIGHT_PASS);
    //scene.add(pointLight);

    //camera.addDebugMesh(); scene.add(camera); // so it shows up in shadow maps

    camera.setPosition(glm::vec3(glm::vec3(0.0f, 5.0f, 50.0f)));

    ngn::Texture renderTexture(ngn::PixelFormat::RGBA_HDR, window.getSize().x, window.getSize().y);
    ngn::Rendertarget renderTarget(renderTexture, ngn::PixelFormat::DEPTH24);

    const int logLumTexRes = 1024;
    ngn::Rendertexture lastLogLumRendertexture(ngn::PixelFormat::R_HDR, logLumTexRes);
    ngn::Rendertexture currLogLumRendertexture(ngn::PixelFormat::R_HDR, logLumTexRes);
    ngn::Rendertexture adaptedLogLumRendertexture(ngn::PixelFormat::R_HDR, logLumTexRes);
    adaptedLogLumRendertexture.setMinFilter(ngn::Texture::MinFilter::LINEAR_MIPMAP_LINEAR);

    // Mainloop
    float lastTime = ngn::getTime();
    bool quit = false;
    window.closeSignal.connect([&quit]() {quit = true;});
    float keyValue = 2.00f;
    while(!quit) {
        float t = ngn::getTime();
        float dt = t - lastTime;
        lastTime = t;

        inputState.update();

        pointLight.setPosition(30.0f * glm::vec3(glm::cos(t), 0.0f, glm::sin(t)) + glm::vec3(0.0f, 10.0f, 0.0f));

        moveCamera(camera, dt);
        camera.updateDebugMesh();

        renderTarget.bind();
        renderer.render(scene, camera, !inputState.key[SDL_SCANCODE_M], !inputState.key[SDL_SCANCODE_N]);

        currLogLumRendertexture.renderTo();
        ngn::PostEffectRender(ngn::Resource::getPrepare<ngn::FragmentShader>("media/shaders/ngn/logluminance.frag"))
            .setUniform("hdrImage", renderTexture);

        lastLogLumRendertexture.renderTo();
        ngn::PostEffectRender(ngn::Resource::getPrepare<ngn::FragmentShader>("media/shaders/ngn/passthrough.frag")).
            setUniform("input", adaptedLogLumRendertexture);

        adaptedLogLumRendertexture.renderTo();
        ngn::PostEffectRender(ngn::Resource::getPrepare<ngn::FragmentShader>("media/shaders/ngn/eyeAdapt.frag"))
            .setUniform("logLuminance", currLogLumRendertexture)
            .setUniform("logLuminanceLast", lastLogLumRendertexture)
            .setUniform("dt", dt);
        adaptedLogLumRendertexture.updateMipmaps();

        ngn::Rendertarget::unbind();
        if(inputState.key[SDL_SCANCODE_UP]) keyValue *= std::pow(2.0f, dt);
        if(inputState.key[SDL_SCANCODE_DOWN]) keyValue *= std::pow(0.5f, dt);
        ngn::PostEffectRender(ngn::Resource::getPrepare<ngn::FragmentShader>("media/shaders/ngn/tonemap.frag"))
            .setUniform("hdrImage", renderTexture)
            .setUniform("logLuminance", adaptedLogLumRendertexture)
            .setUniform("keyValue", keyValue);

        window.updateAndSwap();
        //quit = true;
    }

    // Make a Screenshot when we close
    window.saveScreenshot("screenshot.png");

    return 0;
}