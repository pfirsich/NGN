# Meshes
# Research & Data
Relative Cost of State Changes in the GL:
http://stackoverflow.com/questions/25505996/opengl-state-redundancy-elimination-tree-render-state-priorities

https://www.opengl.org/wiki/Vertex_Specification_Best_Practices

https://developer.apple.com/library/ios/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/TechniquesforWorkingwithVertexData/TechniquesforWorkingwithVertexData.html#//apple_ref/doc/uid/TP40008793-CH107-SW7

## interleaved
tendenziell immer schneller, wegen höherer cache locality

## hintereinander
ist fast immer schlechter?

## multiple buffers
gut, wenn man dynamische meshes hat, die man nur partiell updaten will

------------------------------------------
## direct vs. indexed
http://gamedev.stackexchange.com/questions/110244/is-index-drawing-faster-than-non-index-drawing

http://stackoverflow.com/questions/17503787/buffers-indexed-or-direct-interlaced-or-separate

Angeblich ist indexing free auf der GPU (A Trip Through.. sagt das auch)
Kostet halt nur mehr Speicher, ist aber automatisch fast 100% effizienter sobald jedes Vertex zwei mal verwendet wird (also in der echten Welt, ANDAUERND)
Für prozedurale Meshes, die häufig einfach lineare indexbuffer brauchen, kann man vielleicht einen fallback machen.

### glDrawRangeElements vs. glDrawElements
http://gamedev.stackexchange.com/questions/116789/performance-of-gldrawelements-vs-gldrawrangeelements-vs-gldrawarrays
glDrawRangeElements verwenden, weil man dem treiber vielleicht spart das selbst zu machen (aber der kann das auch besser). just in case, dass efficient data transfer ein problem ist, kann man das aber machen. kostet ja auch nicht viel.

------------------------------------------

## Instancing
braucht nur extra dynamische vertex attributes oder uniform buffers + ein paar extra function calls
http://learnopengl.com/#!Advanced-OpenGL/Instancing
http://sol.gfxile.net/instancing.html

## Batched Geometry (statisch)
multiple meshes in one buffer, drawn at once (to minimize drawcalls)
nur möglich, wenn material gleich ist, also wäre instancing genau so gut? + einfacher zu implementieren, außerdem gibts ja VertexData.merge
man braucht auch primitive restart. also viel schlimmer zu implementieren

## Dynamisch
often with only some attributes changing (being streamed)

## (insane) multiple meshes in one buffer, drawn separately (to minimize buffer overhead) - retarded

## skinned mesh
http://mmmovania.blogspot.de/2012/11/skeletal-animation-and-gpu-skinning.html
extra attributes (statisch - weights und bone ids) + uniform buffers (transformationen)

## shared VBOs
ein bisschen wie batching, hier will man aber die vertex state changes minimieren, also fasst man die hier zusammen und callt dann separate drawcommands.
tendenziell sind vertex format state changes relativ cheap. das ist den aufwand nicht wert. höchstens, wenn man es vollständig automatisieren kann.

-----------------------------------------------------------------

## Alignment

https://www.opengl.org/wiki/Vertex_Specification_Best_Practices#Attribute_sizes
There is something you should watch out for. The alignment of any attribute's data should be no less than 4 bytes. So if you have a vec3 of GLushort​s, you can't use that 4th component for a new attribute (such as a vec2 of GLbyte​s). If you want to pack something into that instead of having useless padding, you need to make it a vec4 of GLushort​s. 

https://groups.google.com/forum/#!topic/webgl-dev-list/lAXfVGRoCzg 
Ohne hin soll das ganze Vertex Format auf 4-Byte-Vielfache aligned werden

------------------------------------------------------------------

# Results
* Indexed Arrays as default with a fallback for simple non-indexed arrays
* Every mesh can have mutiple VBOs so dynamic attributes can be set more efficiently
* Arrays will always be interleaved, because there are helpers to simplify creation and there are very little use cases otherwise
* No multiple objects in a single mesh
* Vertex layouts should be padded automatically (4-byte per attribute and therefore multiples of 4 in total)

-------------------------------------------------------------------

# Implementation Details
* VBOWrapper owns the data in it, since there is probably no use to keep it without the VBO.
    - One constructor that doesn't allocate anything
    - One constructor that allocates space for the data
    - Onc constructor that takes ownership of data passed to it
* mVertexData and mIndexData - The Mesh class has ownership of the objects stored here, seldom it is needed to share VertexData or IndexData objects between meshes. Either you want differennt IndexData with same VertexData, which I cannot think of any use for, or you want different combinations of VertexData, which is also not needed often (or should be needed).

-------------------------------------------------------------------
Why there is no struct/type associated with a VertexFormat.
On the one hand packed stuff or types with weird conversions might be tricky (and some work to implement).
Also you can not just put all the attributes in a struct and hope for them to be contiguous in memory (and without extra stuff or padding) especially not if you derive from a VertexFormat Baseclass that has virtual methods (because of the vtable).
You would have to keep book for every attribute somewhere else anyways.
That would leave the possibilty of doing something like this:
MyVertexStruct* vertex = vertexData["inPosition"];
with MyVertexStruct being a type that maps the vertex format.
Problem: boxMesh, sphereMesh etc. would have to take a template argument, that still would not help them much, since they mostly want to access attributes by hints (e.g. AttributeType::POSITION, etc.). You would need the system already in place anyways.

A way to do it
```c
struct MyVertexStruct {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4<uint8_t> color;

    static void init(cost) {
        vFormat.addAttribute(VertexAttribute::HINT::POSITION, 3, VertexAttribute::TYPE::F32);
        vFormat.addAttribute(...)
    }
    static ngn::VertexFormat vFormat;
}
```

MyVertexStruct::init();
Mesh.addVertexData(num, MyVertexStruct::vFormat, VertexData::USAGEHINT::STATIC)
// returns reference to VertexData
// auto data = VertexData.getData<MyVertexStruct> -> MyVertexStruct*
data[0].position = ...;
Mesh.addIndexData()

Mehr nachdenken über: Meshes laden, 

-------------------------------------------------------------------
Notes for later:
Instancing can be implemented by putting the instance data into a separate VertexData object with usage DYNAMIC or STREAM and repeated calls to upload() (even after Mesh.compile)
Also there is a need for seeting a divisor for an attribute and to set the number of instances of a mesh (Mesh.setInstances(n)).

-------------------------------------------------------------------
Lessons learned
VertexAttribute(hint, num, type, normalized = false) // normalize is rare, hint is the most important actually

-------------------------------------------------------------------
Materials make some problems: 
* For one some Materials might make certain Vertex formats necessary, while others (even other Materials, but of the same MaterialType!!) need different vertex formats. Some might need tangents, some might not.
* Dynamically changing Materials might cause serious issues this way, since different vertex layouts might be required. Either the vertex data will be rebuilt or the  engines just whines loudly and it doesn't work. I think rebuilding and whining a lot is very good, since this is something you don't want to do often anyways. Also there should be mechanisms in place to generate a VertexBuffer format that is compatible with a given number of Materials and the current renderer.
* Also sometimes a depth-only pass can be rendered a lot more efficiently if no unnecessary vertex attributes are transmitted (e.g. normals are not needed).For that it might be worth it to store separate VBOs with only that data. 
* If I keep my system of recompiling VAOs, I will just not AttribPointer the attributes that were removed. -> Custom Shader for Depth-Prepass (position and texCoord for alpha test)
* Also a Mesh potentially containing buffers of different Vertex formats makes it very hard to generate proper mesh data for a given set of Materials. It is better if the VertexFormat contains information about all the Vertex buffers.
Otherwise, just like accessing the vertex attributes checking compatibility with a material, we would have to ask the mesh and not the vertex format, which is just unintuitive. what for is it then? Also the VertexFormat can do sanity checks a lot better (e.g. only one attribute with the position hint etc.)

-------------------------------------------------------------------
Geometry-inbetween.
Also better if you want to do post-processing of geometry (e.g. normalize). It makes access easier and looks more intuitive. 
(Making a mesh out of it, *after* you've processed)
Also this geometry data maps to mesh files more closely.

It gives me room to change stuff later. If there is a GL agnostic representation of the geometry, there are less domain-specifics in the description of the scene. It feels good that GL independent data can be saved GL independent.

If this is present, there is a fully engine independent description of everything in the engine (the scene with transforms, materials that are essentially key-value-store and geometry)

Accessing data is WAY easier, because all fields are predefined (with their types) custom fields are just contiguous stretches of memory that can be users liking. In the end the VBO has to be built from it, but for every type specified type there can be a specific conversion path, since the source data is fixed in type. For normalized integer attributes and packed data (e.g. 2_10_10_10) convenience types should be built. Maybe not full types, but just a from and to function
pack_2_10_10_10_REV_normalized(const glm::vec3&)
unpack_2_10_10_10_REV_normalized(const glm::vec3&)

At the moment you have to be very careful with loading geometry. You load a mesh with tangents and colors, but you don't need them, so you Make a mesh with a VertexFormat that doesn't have them, but later change the material which needs them again. There has to be alot of management, which is not necessary if they would have never been thrown away in the first place.

VertexFormat + Geometry -> Mesh
Mesh + Material -> Object

A Mesh would probably still consist of VertexData and IndexData (btw. just allow multiple of those, because why not)-Objects.

Problem #1: It's not a pretty to load scene trees from files (like ASSIMP)
Potentially this is not possible at all and there will not be a assimpGeometry(filename)-function but only assimpMeshGeometry(aiMesh*) function.

Later a assimp scene loader will then create a scene tree - that also makes more sense, since Materials are going to be loaded as well.

But sometimes you just want a Mesh, a merged one. This is still kind of weird, since you would have to write a collapseSceneTree function or something, which is weird and a huge problem on its own.

Problem #2: What about dynamic attributes?
A Mesh probably needs a reference to the Geometry object. The Geometry object may be deleted, but if you want to use dynamic attributes, you would probably keep it, modify it and trigger a rebuild in the VertexData-Object in the Mesh. This is a lot of memory-copying though and might not be a good idea. It would be better if that manipulation could be done in place, like it is right now.

# Scenarios to get right:
* Load Mesh from file, with a given vertex format
* boxMesh/sphereMesh etc.
* build a custom vertex/(index) buffer with a custom vertex format and fill it
* all of the above scenarios with one extra dynamic buffer
* Load mesh from file and append custom data (dynamic or not)
* Load mesh from file merge with boxMesh/sphereMesh and have dynamic data, some provided by yourself, some generated, also with differend size - for example for instancing
* Just load a mesh, don't provide any other data
* Load a mesh, provide a Material
* Loaded a mesh and make an object that does not need tangents and then later change the material to one that needs tangents
* Just load a mesh, without providing any other data, but normalize it and get the bounding box or something
* Just load a mesh, without making any more decisions and merge it with another loaded mesh
* Load a mesh and dynamically animate one attribute of it (one case: position, another case: custom)
* Without creating an Object (and using a Renderer-Object), load a mesh and draw it with mesh->draw() (if that exists)

Where can LOD fit into this? Where can Instancing fit into this? Where Skinning?

https://docs.unity3d.com/Manual/GPUInstancing.html
Automatic Instancing is very hard to detect and I don't really want to make design decisions that cripple parts of the engine just to make this detection possible. Maybe there should be an InstancedMesh-subclass, that somehow takes care of this? Maybe we can detect if a draw call is the last one for a specific mesh and ignore all the previous ones, while saving the necessary data for the last call? How would one add per-instance extra-data? For example even Material data? Probably by registering a certain uniform as "per-instance" and the InstancedMesh-class then extracts them from the uniform-list as well. In forward rendering multipass rendering either all light information has to be be per-instance data (which is impossible with different lights and a only one light per object, even if it was not multi-pass the per-instance vertex attributes would be huge - also lots of this would be easier if not vertex attributes, but uniform buffers would be used for this)
Correction: Multipass forward rendering still works and would still profit from instancing (a lot).

---------------- experiments
```
struct MyVertexStruct {
    glm::vec3 position;
    uint32_t texCoord;

    static void setupBufferFormat(const VertexBufferFormat& format) {
        format.
        addAttribute(VertexAttribute::HINT::POSITION, 3, VertexAttribute::TYPE::F32).
        addAttribute(VertexAttribute::HINT::TEXCOORD0, 2, VertexAttribute::TYPE::UI16, true);
    }
}
```

ngn::VertexFormat vFormat;
MyVertexStruct::setupBufferFormat(vFormat.addBuffer(STATIC));
vFormat.addBuffer(DYNAMIC).
addAttribute(VertexAttribute::HINT::CUSTOM0, 1, VertexAttribute::TYPE::UI2_10_10_10_REV);

ngn::Mesh* mesh = new ngn::Mesh()

Always copy VertexFormat by value.

-------------------------- Realizations from lying in bed last night
* boxMesh und sphereMesh könnten ein paar Spezialisierungen sparen, indem man die positions z.b. immer nur als vec3 oder vec4 schreibt. Ist ohnehin für die meisten Sachen so.
* So oder so, braucht man immer noch diese Accessor-scheiße. Ich weiß nicht wie man einen VertexDaten-Buffer mit vorgegebenem Format aus bestehenden Daten bauen kann, wenn man nicht so einen Mechanismus in place hat. Ich glaube da wird man nicht los. Und wenn man es nicht los wird, dann kann man sich das struct auch sparen.
* Vielleicht nur die Geometry-Klasse einfügen, damit man eine gute Grundlage hat, die in VertexBuffer streamen kann oder so.
* VertexData und IndexData werden zu VertexBuffer und IndexBuffer umbenannt. Mesh ist dann effektiv eine neue VertexData-Klasse die alles beinhaltet. Die Nummer mit den structs kann man immer noch mit .getData() -> pointer irgendwie umsetzen (+ cast)
* Die Geometry-Klasse braucht man eigentlich immer noch nicht. Außer man macht automatische Generierung. Und das halte ich ohnehin für gefährlich, weil man das eigentlich nur verkacken kann und andauernd recompiles triggert oder Sachen x mal rumfliegen hat. Wichtig ist, glaube ich, eher, dass man einen guten VertexFormat-Generator aus Materials hat. Man spart so viel Speicher und Kopiererei und dafür denkt man einfach ein bisschen mal mehr nach. Wenn man kein automatisches generieren hat, ist die Geometry-Klasse ziemlich useless. Genau so könnte man einfach ein Mesh mit einem größeren VertexFormat nehmen und dann convertVertexFormat machen (was es ohnehin geben muss). Damit könnte man sogar automatisch recompilen und vllt. auch generieren oder jammern. Mesh-Operationen kann man auch einfach auf dem Mesh-Objekt machen, bevor man es uploaded halt. Das ganze Ding laden, plump speichern und dann rauskopieren und interleaven ist einfach unnötige Arbeit, die man tatsächlich sooo selten machen will. ALLES BLEIB WIES IS.
* Wenn man VertexFormat zu VertexBufferFormat und VertexFormat macht, dann kommt man an eine Stelle, an der man, z.B. für instancing, nicht einfach VertexFormat.allocate() machen kann, sondern für einzelne Buffer einzeln allozieren will (oder dafür einen komischen extra-mechanismus haben muss). Man muss also die Funktionalität, wie sie jetzt ist ohnehin bereitstellen. Das ganze ist dann nur ein helper für ein paar use-cases, die leider auch die häufigsten sind, der die Klassenhierarche irgendwie viel undurchsichtiger macht. Lässt man es weg, muss man halt damit leben, dass man hasAttribute einfach aufs Mesh macht. Das Mesh muss ein bisschen aufwändigere sanity checks machen. Wenn man ein VertexFormat generieren will, dann fällt mir kein Fall ein indem man die Daten in versch. VertxBuffern liegen haben will. Maximal bei dynamischen attributen, von denen man aber wissen sollte und man könnte die extra auffangen.
* Wenn jedes Attribut ein Hint braucht, dann kann man die Namen weglassen und das enum auf ein int mappen, dass global die location bedeutet, sodass man immer konsistent glBindAttribLocation sinnvoll aufrufen kann und die connection mit den namen. Man braucht nur noch hints. Bei glAttribLocation braucht man eigentlich noch nen namen. Vielleicht will man an den Shader auch einen haufen #defines + #line 0 prependen, damit man sowas machen kann:
layout(location = POSITION_ATTRIBUTE) vec3 position; (ausprobier, funktioniert)

später:
IndexBuffer sollten einsam bleiben. Man macht damit nur möglich, dass man Kram macht, der einfach schlechter ist. Das Material kann man ohnehin nicht wechseln, also ist ein IndexBuffer genau äquivalent zu mehreren, mal abgesehen davon, dass es effizienter ist nur einen zu haben!

=> Fazit:
Alles bleibt, wie es ist, bis auf ein paar Kleinigkeiten
neu:
[ ] VertexFormat.removeAttribute
[ ] VertexFormat.require(std::vector<AttributeType>)
[ ] Mesh.supportsMaterial / hasAttributes
[x] Rename IndexData and VertexData to IndexBuffer and VertexBuffer
[x] VBOWrapper to GLBuffer
[ ] make sure that convertVertexFormat can be implemented as I think, maybe even do it already (naaah). - Das ist vermutlich auch etwas komisch, wenn man mehrere VertexBuffer hat. Ich denke stattdessen will man einfach eine Funktion haben, die einen VertexBuffer mit den Daten von einem anderen füllt (wenn vorhanden und mit Konvertierung) - 
[ ] VertexAttribute-Namen ditchen und nur den Type nehmen
[ ] make sure VertexFormats are copied whenever used