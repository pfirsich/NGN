# Materials

## Random Notes
* There has to be a distinction between a container for all necessary values / textures for a given "Material" and the "Material" itself, being the logic to handle this data. This logic has to have the MaterialData as an input and should be able to build shaders, setup state, send uniforms, etc. => Terminology! => Material and MaterialType? (in pairs)
* Should this MaterialType be a class? Or just a function?
* Can this MaterialType be data? It just has to map input to output values. Assuming GLSL is data, this could theoretically be data.
* In Unreal this seems to be solved with a list of input values (could by any number or map) + some predefined inputs (world position, normal, etc.) and an output value, inbetween is node-stuff, but could easily be GLSL in my version. Just has to be abstracted away. If I use an interpolated vertex normal or the normal of a normal map, should not change much. This can be hard.
* Every input parameter should be able to be a value or a map
* There would have to be measures in place, that check if the described materialtype is possible in the current renderer. e.g. a deferred renderer will not be able to hold dozens of parameters (material-specific data), but only a very limited amount. The renderer gbuffer layout should be configurable anyways and provide "CUSTOM0" to "CUSTOMN", which can then be used by the material-pipeline. There has to be a way to specify bits used for material-id. If the G-Buffer doesn't store that, then there can only be one material.
* PossibleInterface:
class MaterialType, Instantiate it for every type. 
MaterialType("Phong", vertexShaderCode, fragmentShaderCode)
PhongMaterial : public Material, with virtual methods to setup uniform state, etc.
Material-Constructor should take a MaterialType-Object
* Depending on some property being a map or a value, MaterialType-Instances have to compile multiple shaders.
* A deferred renderer would have to check the number of map parameters, then query them, so it can unpack them with the right names. 
* Therefore MaterialType and Renderer have to be coupled, because the resulting shaders are different.

!materialtype
!parameter type=float min=0.0 max=0.0 map=true value=false: specularity
!parameter type=float map=true value=true: specularExponent
!glsl fragment:
ngn_albedo * dot(ngn_lightDirection, ngn_normal)

!material
!specularity: kaka.png

* In the Model described: 
    - How do Vertex Attributes as parameters fit into this picture? - Normals are kinda crazy, since they need one attribute sometimes and if they are a map, they need two. Not every attribute is easily replaced with a map. (Scalars might be)
    - This also shows a problem: VertexData of Meshes has to be partially regenerated (for example tangents) for different Materials! Also the vertex format has to be changed, depending on the material! FOR FUCKS SAKE.
* If only ngn_lightDir, ngn_lightColor and ngn_lightAttenuation are provided spot, directional, point are easy to do. Maybe encourage using ngn_NdotL as the main variable, but also expose ngn_lightDir and with hemisphere lights these get overridden:
ngn_NdotL = 1.0
ngn_lightAttenuation = 1.0
ngn_lightColor = mix(_light_groundColor, _light_skyColor, dot(ngn_lightDir, ngn_normal) * 0.5 + 0.5)
* Image Based Lighting/Spherical harmonics would replace the ambient-term (with a more complex function), ambient term should be computed by the backend?
* In theory for Forward rendering different lights should be able to be batched with a uniform array (i.e. the renderer will create a loop over multiple lights and unpack the values in to ngn_lightDir, ngn_lightAttenuation and add them or just do it multipass)

* This is how Materials are edited in Unreal: https://wiki.unrealengine.com/Intro_To_Materials_(Tutorial)

## Options

### Fully Data-Driven Approach
In this case a "Material" is just a set of parameters and a type name. For example PhongMaterial, which only contains the necessary parameters for PhongLighting + shadow + regular state changes, etc.

#### Pros
Materials are very lightweight, renderers can do clever things for certain materials

#### Cons
Data is not everything. There has to be logic somewhere and it would be a shame, if you would have to subclass the renderer and put in extra clauses, if you want to add another material

--------------------------
## Example Flow

### Preparation
class PhongMaterial : public Material {
public:
    glm::vec3 diffuse;
    float shininess;

    ShaderProgram getShaderProgram()
}

### Usage
Geometry* box = boxGeometry(100, 100, 100);

PhongMaterial mat;
mat.setDiffuse()

**either**:
VertexFormat fmt;
fmt.require(mat);
fmt.require(otherMat);

Mesh* mesh = fmt.makeMesh(box);

Object* obj = new Object(mesh, mat);

-> This constructor will check if the mesh is compatible with the material, then VAO is built for the material.

**or**:
Object* obj = new Object(box, mat);

-> This constructor will automatically do what we did in the "either" section, but only for one material

### Shader building
I think it would be a good idea if there was a slot-system for everything and it could be overridden. The Material base class just asks the current renderer for the default content of the slot and the material can override it.

At the least a Material (a Material Type and a set of parameters) should correspond to a single shader. What I don't like is that you may modify the Material values on a per-instance basis, which might trigger recompiles or shader switches. Maybe you have to compile all combinations beforehand? Or debug output the recompile so that the user of the engine can put measures in place to compile the necessary versions ahead of time. 

OR

There is one Shader per MaterialType and proper default values (for example a default normal map - one pixel blue) are provided. For things like shininess both a texture and uniform value are passed to the shader where they are always multiplied. If no shininess texture is set, but rather a single value, the default texture is white. Or add them and bind no texture and add them (according to the spec fetch an unbound sampler will always result in (0, 0, 0, 1)). In that case there should be a file for a MaterialType and files for the Material. Doing it this way makes it easier to instance models, because the same Mesh and the same Material is guaranteed to make it possible to instance them, i.e. they reference the same model. In that case though Material properties have to be instanced as well, or all the properties have to hashed and instancing is only happening, when the uniforms are equal. This should be done one way or another anyways to minimize uniform uploads. Maybe Instancing should happen explicitly?

You need multiple shader programs anyways, since you want to render different light types for example. And maybe another shader for depth prepass or something.

Material just as MaterialTypes should be Objects, so they can fit easier into a data driven approach.

Maybe optimize Uniform-Calls

The Renderer should initialize a UniformBlock-Object that anyone involved (the SceneNode, the Camera, the Material, etc.) can add Uniforms to.

In any case is a good idea to have Material-Descriptions only be a reference to a lighting model and an aggregation of parameters necessary for it.

Recompiles are actually really bad. Either whine a lot, when they happen, but do it. Or don't do it automatically, but explicitely?

Maybe just throw away the idea of renderer agnostic materials? At least provide a path to just replace everything and document what is needed for it. Probably even implement that first!
Maybe defining material types for a single renderer is fine as well?
BOTH SHOULD EXIST

material:
    name: Wood
    type: Phong
    ...

materialType:
    name: Phong
    renderers:
        Forward:
            renderstate:
                depthTest: true
            passes:
                pointlight
        Deferred:
            passes:
                gbuffer:
                    ...
                pointlight:
                    glsl: &lightglsl |
                        #define blabla
                        !include "pointlight.glsl"
                        getLightParams -> lightDirection, lightAttenuation...
                spotlight:
                    glsl: *lightglsl


It is really, really important, that the Material system is usable out of the box for basic shading.
A guy on gamedev.net argued (and is right) that if you want to make a single game, you should have one type of material (which is bullshit) but the point stands that being able to do everything in a single way without doing much work is important.

Vielleicht ist ein Material einfach ein Objekt, dass mir ein Shader-Objekt gibt, wenn ich sage "ich hätte gerne technique X, pass Y" und zusätzlich in der Lage ist ein UniformBuffer-Objekt zu konstruieren, dass die nötigen Parameter hält, die ich dann übergeben darf.
Am besten kann man da auch beliebige Objekte zurückgeben, die von UniformBuffer erben, aber schöneren access zulassen: setDiffuseColor statt
setParameter("diffuseColor", glm::vec4());
http://www.gamedev.net/topic/678502-best-way-to-abstract-shaders-in-a-small-engine/ - Ingenu
vielleicht muss es sowohl eine UniformMap geben, die glUniform-calls macht, als auch Buffer (die vorzuziehen sind, aber für manche anwendungsgebiete halt cancer sind)

Z-Prepass: sort front to back
Lighting: sort by material
Translucent: back to front

VertexFormat bekommen indem man aus dem Material den Vertex Shader holt und daraus die Liste von Vertex-Attributen


Erst Graph traversieren (es wäre gut, wenn der linear in memory liegt)

OGRE:
properties:
ambient, diffuse, specular, emmissive, shininess, txtures, sourceblendfactor
depth test, depth write, depth comparison, cull direction (ccw, cw)
dynamic lighting enabled
polygon mode (lines, fill, points)

technique properties:
listed in order of preference. earlier techniques are preferred over later ones
attributes: 
lod_index/lod_distance - 0 is best quality and default
shader_caster_material - you can specify another material for shadow map rendering
shadow_receiver_material - 
gpu_vendor_rule, feste liste mit include oder exclude
gpu_device_rule, pattern-matching

sampler properties:
texture_wrap_mode: repeat, clamp, mirror, border
filtering: none, bilinear, trilinear, anisotropic
max_anisotropy
mipmap_bias

material top level properties:
lod_values distanc1, distance2, distance3
receive_shadows

außerdem gibts nen haufen default vertex/fragment programs, sodass man super viel darin schon einfach machen kann (selbst ohne zu erben, aber das mag ich nicht)


material inheritance, that literally just copies and you can overwrite stuff

entweder explizit ne materialtype klasse und material-instanzen, die nur uniforms halten
oder material hält halt nur parameter + referenzen zu shader-objekten. viel mehr isses nicht, aber wenn man vversch. kopien hat könnte man blöd werden.

In OGRE können jede Semgnete des scripts inheriten (mit der gleichen syntax) von allen anderen genamedten segmenten.

Vielleicht will man parameter im material markieren um anzudeuten, dass man sie nicht während der laufzeit ändern möchte. dann können die in den shader kompiliert werden.


renderer queue entry:
ShaderProgram*
vector UniformMaps
vector UniformBuffers
texture unit map
Mesh* - alternatively VAO, mode, vertexCount, indexCount, etc., but why? (probably because of cache)
RenderState

generates a sort-key or implements comparison

This might be the holy grail:
http://www.slideshare.net/repii/frostbite-rendering-architecture-and-realtime-procedural-shading-texturing-techniques-presentation?from=ss_embed

The Disney BRDF is used for EVERY material (except hair I think) in their movies, which makes this obviously a little easier. 


--- Vertex Formats
http://www.gamedev.net/topic/668726-graphics-engines-whats-more-common-standard-vertex-structure-or-dynamic-based-on-context/
In the final case a material would reference a vertex format, which is also a data file and the mesh would already be saved in that format for maximum efficiency.
Also a mesh probably needs multiple VAOs for different shaders (for example depth-prepass only binds pos/maybe uv)
Mostly the cost for paying the whole vertex buffer is payed, but having a separate VAO just for shadow maps/z-prepass is a good idea.
But best (and I want this) is to use two streams, one for shadow maps and z-prepass and another stream for all the other data (normals, tangents, colors, etc.)
The meshes should be exported with information about the vertexformat they have been exported with, so that a check can be performed.

----
Blending, Depth Function, Cull Face Side, Front Face, Stencil Function, Stencil Operation

--- regarding UBOs
http://www.gamedev.net/topic/655969-speed-gluniform-vs-uniform-buffer-objects/
Der Speedup ist, selbst, wenn man es richtig macht, nur sehr klein, also np einfach normale Uniforms zu benutzen (erstmal)
Später dann:
http://sunandblackcat.com/tipFullView.php?l=eng&topicid=21&topic=OpenGL-Uniform-Buffer-Objects
http://learnopengl.com/#!Advanced-OpenGL/Advanced-GLSL

Urho:
different passes used in different renderers/stages
texture and shader parameters defined on the technique level
technique has a quality (0-2)
texture-attribute also holds sampler state
shaders and defines can be specified on technique and pass level
no different techniques for skinned, non-skinned, instanced, billboard, different pervertex/perpixel lights, but rather defines

passes:
a switch for unlit|perpixel|pervertex

in deferred:
deferred - rendering in to gbuffer
emissive - additional pass for emmisive after light accum

in light prepass:
prepass - opaque geometry normals and depth to g-buffer
material - rendering opaque geometry for second time using light accum result

in forward:
ambient: base pass for opaque or transparent geometry
negative: a darkening pass used for negative lights
light: additive pass for a single light

for all:
postopaque - custom forward pass after standard opaque before transparent
shadow - rendering to shadow map

refract - after postopaque that renders the viewport texture from the environment texture unit, will ping pong the scene rendertarget, but will not be performed if not refractive stuff to render

base (ambient, per-vertex and fog)
light: one per pixel light for opaque object
alpha: base for transparent
litalpha: light for transparent

default render state: 
alpha amsking and testing disabled
blend mode: replace
ccw culling
depth test: lequal
depth write enabled

for quality and  lod to work correctly the techniques have to appar in specific order:
most distant & highest quality
...
most distant & lowest quality
second most distant & highest quality
...

base materials will be copied completely before overwriting selectively



urho uses an ubershader-like approach, permutations are built on demand
inbuilt defines and specified defines

defines in VS:
NUMVERTEXLIGHTS
DIRLIGHT, SPOTLIGHT, POINTLIGHT, PERPIXEL
SHADOW (light has shadowing)
NORMALOFFSET
SKINNED, INSTANCED, BILLBOARD

PS:
DIRLIGHT, SPOTLIGHT, POINTLIGHT, PERPIXEL
CUBEMASK (mask for spotlight)
SPEC - light has specular
SHADOW
SIMPLE_SHADOW, PCF_SHADOW, VSM_SHADOW
HEIGHTFOG

helper functions like GetWorldPos, GetClipPos

shader precaching can be done beforehand for a predefined set of combinations

Effektiv einfach eine Liste Shader haben, mit Tags, die dann entsprechend ausgewählt werden können.

```yaml
material:
    name: Phong # not needed, filename
    #base: # filename
    lodDistances: [12, 100]
    techniques:
        -   tags: []
            lodIndex:
            parameters:
                albedo:
                normalMap:
                specularIntensity: # value or map
                specularExponent: # value or map
                specularColor:
            shader:
                vertex: # can be auto-generated
                fragment:
                    includes:
                    defines:
                    code: !!file path # maybe only files?!
                    code: |
                    int main(...) {
                        vec3 
                    }
```

Use Material Inheritance instead of Material Type and Material?
Is that enough?
Ist es vielleicht cooler einen Parameter-Satz "Material" zu nennen. Und dann eine Reihe Shader dafür zu haben, die man referenziert und die auch darauf passen müssen.

-----

Unity hat seit einer Weile "surface" shader, die als output nur eine Reihe üblicher Parameter haben, die dann durch das lighting model später verwurstet werden (an beliebigen stellen der pipeline):

Input ist:
float3 viewDir
float4 with COLOR semantic
float4 screenPos
float3 worldPos
float3 worldRefl
float3 worldNormal

Output:
struct SurfaceOutputStandard
{
    fixed3 Albedo;      // base (diffuse or specular) color
    fixed3 Normal;      // tangent space normal, if written
    half3 Emission;
    half Metallic;      // 0=non-metal, 1=metal
    half Smoothness;    // 0=rough, 1=smooth
    half Occlusion;     // occlusion (default 1)
    fixed Alpha;        // alpha for transparencies
};
struct SurfaceOutputStandardSpecular
{
    fixed3 Albedo;      // diffuse color
    fixed3 Specular;    // specular color
    fixed3 Normal;      // tangent space normal, if written
    half3 Emission;
    half Smoothness;    // 0=rough, 1=smooth
    half Occlusion;     // occlusion (default 1)
    fixed Alpha;        // alpha for transparencies
};

früher:
struct SurfaceOutput
{
    fixed3 Albedo;  // diffuse color
    fixed3 Normal;  // tangent space normal, if written
    fixed3 Emission;
    half Specular;  // specular power in 0..1 range
    fixed Gloss;    // specular intensity
    fixed Alpha;    // alpha for transparencies
};

Man kann auch eine "finalColorModifier"-function angeben, die die color NACH dem lighting beeinflusst. 

man kann auch einen vertex-shader definieren

Das ist eigentlich ziemlich geil.

Wenn man das Lighting Model ändern möchte, kann man dafür Funktionen definieren, die ein SurfaceOutput + light dir + viewdir (optional) + light atten. Output ist ein half4, also eine einzige color. An dieser Stelle kommen auch lightmaps ins Spiel? Eventuell?

Diese sind auch light-type-agnostisch! (oho, boys!)

Wenn man die SurfaceOutput-Structs selbst definieren kann, kann man damit implizit das G-Buffer layout partiell beschreiben und mit ein paar extra-angaben auch komplett. 
In Unity kann man sogar das Lighting Model für Deferred gar nicht ändern.

man sollte materials als "forwardonly" taggen können, die dann im deferred renderer in einem post-opaque-pass ausgeführt werden (oder post-alpha, wenn sie alpha sind)

Die Shader-Objekte kann man dann auch für Postprocesses und so benutzen und die ShaderProgram-Abstraktion, die momentan existiert ersetzen

vllt. kann man in jedem shader einfach bestimmte funktionen definieren, die extrahiert werden können. in surface-shadern definiert man eine surface-funktion und überschreibt eine existierende vom renderer bereitgestellte default-implementation von fragment() und vertex() nicht, kann die aber alle überschreiben (dafür muss man magic machen, glaub ich). so kann man vllt. auch lightingmodel überschreiben.

Materials sind tatsächlich nur Parameter-Sätze für die Shader. Wichtig: Ein Material, dass mit mehreren Objekten assoziiert ist und geändert wird, ändert sich für alle diese Objekte. -> Uniform Buffer Objects! besonders, wenn sie statisch sind.

----- 
Recap:
class Material
class Shader - hat eine Liste von implementierten Funktionen?
Vielleicht muss man Funktionen auch registrieren? Entweder auf ne Meta-Ebene 'setSurfaceFunction("surface")', oder mit ner extra-directive "!surface", damit kann man dann auch includes schön regeln, oder durch parsen introspecten.
Die Meta-Nummer ist vermutlich am besten zuerst zu implementieren, weil es wenig Arbeit ist und weil man so ähnliche Funktionen sowieso braucht, falls man directives implementieren sollte (die mir am besten gefallen).

Forward Renderer (mit allem Krempel)
Deferred Renderer mit versch. Lighting Models - einfach nur einzelne versch, aber auch wenn man mehrere auf einmal will und einen lightingmodelindex einbauen muss

versch. Effekte:
* No Man's Sky - Topographic Scanner: In Deferred als Post-Process, in Forward muss das ein Material sein, dass jedes Objekt hat, oder partiell implementiert.
* Winston's Barrier - post-alpha sogar wegen refraction, screen als input-texture, refraction muss man ohnehin überdenken
* Feuer?
* Screen Space Reflected stuff?
* Es sollte die Möglichkeit geben ein Objekt mit fest definierbaren vertex und fragment shadern an bestimmten Stellen/passes in der pipeline zu rendern (dann renderer-abhängig)

in Unity kann man AN das OBJEKT (nicht im Material) kleben wie gesortet werden soll, cast shadows, receive shadows, sorting layer und order in layer (vielleicht ist das aber nur für billboard)
Blending ist aber z.B. im Material

RenderStateBlock -> setDepthTest(), 
UniformBlock -> UniformSet, UniformBuffer - Interface: ->setFloat(), ->setInteger(), etc. und bind() - lazy upload für buffers

RenderStateBlock.addState<RenderStateBlock::DepthTest>(true);
RenderStateBlock.removeState<RenderStateBlock>();
RenderStateBlock.getState<RenderStateBlock>().set(false);

Unreal Materials:
https://docs.unrealengine.com/latest/INT/Engine/Rendering/Materials/MaterialProperties/index.html
https://docs.unrealengine.com/latest/INT/Engine/Rendering/Materials/HowTo/Refraction/

Es scheint als gebe es hier keine Möglichkeit, wie in Unity, für jeden Renderer/Pass einen eigenen, vollen Satz Shader (frag+vert) anzugeben, man hat nur die high-level-abstraction (mit surface und lighting model)

Shader permuations: http://www.gamedev.net/topic/675184-shader-permutations/

--------------------------------------

```c++
class Pass {
    BlendMode mBlendMode;
    RenderStateBlock mStateBlock;
};

class Material : public UniformList {
    RenderStateBlock mStateBlock;
    std::vector<std::pair<PassIndex, Pass> > mPasses;

    ShaderProgram* getShader(PassIndex pass)
};
```

im shader dann:
```c++
#define NGN_PASS_FORWARD_AMBIENT 1
#define NGN_PASS_FORWARD_LIGHT 2

#define NGN_PASS_RENDERER_DEFERRED(x) (x >= 100)
#define NGN_PASS_DEFERRED_GBUFFER 100
#define NGN_PASS_DEFERRED_LIGHTING 101

[...]
#define NGN_PASS NGN_PASS_FORWARD_AMBIENT
[...]
#if NGN_PASS == NGN_PASS_FORWARD_AMBIENT
stuff
#endif
```

Dann gibt es z.B. ein BaseBlinnPhongMaterial, dass einen surface-type und ein standard-lightingmodel + main implementiert.

Wenn ein Material davon erbt, kann man dann Funktionen überschreiben

Weil man nicht für jedes Material separat shader program objects cachen möchte, sollte man diese Generierung an eine Extra-Klasse delegieren, die Shader-Klasse