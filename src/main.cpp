#include <ngn/ngn.hpp>

int main(int argc, char** argv) {
    ngn::setupDefaultLogging();
    auto window = new ngn::Window("ngn test", 1600, 900);
    auto size = window->getSize();
    auto renderer = new ngn::Renderer();
    renderer->viewport = glm::ivec4(0, 0, size);
    auto camera = new ngn::PerspectiveCamera(45.0f, (float)size.x / size.y, 0.1f, 100.0f);
    window->resizeSignal.connect([&](int w, int h) {
        renderer->viewport = glm::ivec4(0, 0, w, h);
        camera->setAspect((float)w/h);
    });

    auto scene = new ngn::Scene();

    auto obj = new ngn::Object();
    scene->add(obj);

    bool quit = false;
    window->closeSignal.connect([&]() {quit = true;});
    while(!quit) {
        renderer->render(*scene, *camera);
        window->updateAndSwap();
    }

    delete window;

    return 0;
}