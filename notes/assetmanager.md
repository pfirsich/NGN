    getResource<Texture>(filename) or (filename, aliasshortname)
    .free - shared_ptr everywhere, if a resource is not found, substitute a replacement (checker texture, error model, identity shader)
    automatic reloading when files have changed?!
    preloading assets (in groups?)
    asynchronous loading
    http://gamedev.stackexchange.com/questions/34051/how-should-i-structure-an-extensible-asset-loading-system
    http://gamedev.stackexchange.com/questions/2185/resource-managers-are-they-any-good
    http://gamedev.stackexchange.com/questions/1702/how-to-design-an-assetmanager
------

AssetGroup?

AssetGroup->add("shortname", path);

AssetGroup->get<Texture>(path)
AssetGroup->get<Texture>(shortname)
AssetGroup->get<Texture>(shortname, path, bool async = false)

AssetGroup->release(name)

AssetGroup->reloadChanged();

------

Was management entgeht, entweder reference counting, was ich aber doof finde für z.b. gegner-typen, die  man tötet und kurz darauf respawned werden oder so. sowas passiert die ganze zeit!

Alternativ: AssetManager->fence()
alles was zwischen zwei fences ist ist dan eine group, die man separat freen kann oder so (noch etwas unkonkret)

oder man macht explizit asset-gruppen, die man auf einmal freen kann

Das scheint irgendwie super game-spezifisch zu sein, vielleicht will man kein catch-all haben? GTA will was anderes als ein Roguelike.

In GTA wäre es vielleicht gut, wenn man mehrere Sets/Groups haben kann, die assets beinhalten, und diese auch teilen könnten! und dann kann man die separat laden (und dann natürlich nichts doppelt laden) und auch freen kann (dann wird nichts gefreet, was von einer anderen group verwendet wird).

Das geht nur, wenn man die assets kennt. Man muss wohl irgendwie eine immediate-layer einbauen können.

Also sollte es einen globalen eigentlichen Manager für Ressourcen geben und obendrauf sind AssetGroups, die auch on demand laden können und die Dinger kann man als ganze sets preloaden oder releasen (re-implementieren im prinzip den globalen manager)
Das ist im prinzip wie reference counting, aber die references halten nicht scenenodes, sondern gruppen (UND scenenodes zusätzlich, im allgemeinen - leider)

-------------

"    /*
    TODO: Maybe give SceneNode and stuff only "ResourceHandles" (a pointer wrapper), that know if the resource they are pointing to
    was released (probably by accident), so they can return a placeholder resource instead.
    if this is handled in the resource-destructor in theory no one could ever point to a non-existent resource

    Also it would then be possible to hot reload assets without them having to implement a "reload()" function or similar, because
    then the original resource could just be destroyed, a new one could be loaded and the handle would return the new one instead
    alternative solution: make sure load() can be called more than once

    Also if default placeholder resources want to be used, then every setFoo(getResource<Foo>("bar")) would have to be replaced with a conditonal
    (also they must not be forgotten!) that assigns the placeholders if the load failed or all setFoo-methods would have to implement this behaviour

    Another option would be for getResource to somehow notify the caller about the success and do the replacement itself, but that would either
    require multiple return parameters (which is a pain to use), or exceptions or a state, which is all very not-pretty

    With ResourceHandles it could just overload a boolean operator or implement .success()/.loaded() and return the placeholder if still
    asked for the resource pointer

    ResourceHandles would represent some kind of future-structure that would have to be used anyways if asynchronous loading is intended to be used.

    Maybe there is an option to have more control over this in the sense that you can control what resource the handle is pointing too until the real resource
    is ready. So while it is loading (in another thread) you have a placeholder, that might a lower LOD version of it.
    */

    /*ResourceHandle myTextureRef("tex.png");
    if(!myTextureRef.loaded()) makeTrouble();
    mat->setTexture("bla", std::move(myTextureRef));
    Texture* myTex = myTextureRef.getResource<Texture*>(); // this will always be a valid resource pointer! Even if loaded() == false!

    node->setMaterial(ResourceHandle("tex.png"));
    node->setMaterial("tex.png"); // should work as well with implicit conversion

    ResourceHandle::collect();
    ResourceHandle::reloadChanged();

    All these options also with aliases!
    Also async loading and explicit placeholders (only in the case of async?)*/

    /*
    Introduce preload vs. load. preload will be the part that can be run in a separate thread (and should explicitely be written that way)
    load will then "commit" to the object itself and create opengl state (which has to happen in the main thread). load should be as small/fast
    as possible
    */"