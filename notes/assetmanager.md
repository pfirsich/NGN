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