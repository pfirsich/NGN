every parameter can be a map or a value
normal mapping requires an additional vertex attribute, is this a problem?
do i have to introduces defines for these types of differences?
Actually if I just recompile, then the attributes are no problem if I query them later, since they were optimized away.
setting a default value for a normal map and being able to convolute them (e.g. roughness value + roughness-map are always used, but they are added and the default map is black or something) then I won't throw away much?

a measure in place to check if the material is renderer-supported?

vFormat.require(Material)? Maybe automatic VAO recompilation for each shader
(it's not that slow) and then "prepare" instead of draw or something

not sure anymore how my material-system works with deferred shading :/

----------

It is really, really important, that the Material system is usable out of the box for basic shading.
A guy on gamedev.net argued (and is right) that if you want to make a single game, you should have one type of material (which is bullshit) but the point stands that being able to do everything in a single way without doing much work is important.

OGRE - properties:
ambient, diffuse, specular, emmissive, shininess, txtures, sourceblendfactor

weitere technique properties:
lod_index/lod_distance - 0 is best quality and default
shader_caster_material - you can specify another material for shadow map rendering
shadow_receiver_material - 
gpu_vendor_rule, feste liste mit include oder exclude
gpu_device_rule, pattern-matching
receive_shadows, cast_shadows

sampler properties (maybe introduce an intermediate class - Sampler to hold this):
texture_wrap_mode: repeat, clamp, mirror, border
filtering: none, bilinear, trilinear, anisotropic
max_anisotropy
mipmap_bias

http://www.slideshare.net/repii/frostbite-rendering-architecture-and-realtime-procedural-shading-texturing-techniques-presentation?from=ss_embed
The Disney BRDF is used for EVERY material (except hair I think) in their movies, which makes this obviously a little easier. 

--- Vertex Formats
http://www.gamedev.net/topic/668726-graphics-engines-whats-more-common-standard-vertex-structure-or-dynamic-based-on-context/
In the final case a material would reference a vertex format, which is also a data file and the mesh would already be saved in that format for maximum efficiency.
Also a mesh probably needs multiple VAOs for different shaders (for example depth-prepass only binds pos/maybe uv)
Mostly the cost for paying the whole vertex buffer is payed, but having a separate VAO just for shadow maps/z-prepass is a good idea.
But best (and I want this) is to use two streams, one for shadow maps and z-prepass and another stream for all the other data (normals, tangents, colors, etc.)
The meshes should be exported with information about the vertexformat they have been exported with, so that a check can be performed.

# URHO
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

--------------------------------

versch. Effekte:
* No Man's Sky - Topographic Scanner: In Deferred als Post-Process, in Forward muss das ein Material sein, dass jedes Objekt hat, oder partiell implementiert.
* Winston's Barrier - post-alpha sogar wegen refraction, screen als input-texture, refraction muss man ohnehin überdenken
* Feuer?
* Screen Space Reflected stuff?
* Es sollte die Möglichkeit geben ein Objekt mit fest definierbaren vertex und fragment shadern an bestimmten Stellen/passes in der pipeline zu rendern (dann renderer-abhängig)

-----

# Unity 
hat seit einer Weile "surface" shader, die als output nur eine Reihe üblicher Parameter haben, die dann durch das lighting model später verwurstet werden (an beliebigen stellen der pipeline):

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

in Unity kann man AN das OBJEKT (nicht im Material) kleben wie gesortet werden soll, cast shadows, receive shadows, sorting layer und order in layer (vielleicht ist das aber nur für billboard)
Blending ist aber z.B. im Material

# Unreal Materials:
https://docs.unrealengine.com/latest/INT/Engine/Rendering/Materials/MaterialProperties/index.html
https://docs.unrealengine.com/latest/INT/Engine/Rendering/Materials/HowTo/Refraction/

Es scheint als gebe es hier keine Möglichkeit, wie in Unity, für jeden Renderer/Pass einen eigenen, vollen Satz Shader (frag+vert) anzugeben, man hat nur die high-level-abstraction (mit surface und lighting model)

Shader permuations: http://www.gamedev.net/topic/675184-shader-permutations/