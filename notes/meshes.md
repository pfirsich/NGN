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
Notes for later:
Instancing can be implemented by putting the instance data into a separate VertexData object with usage DYNAMIC or STREAM and repeated calls to upload() (even after Mesh.compile)
Also there is a need for seeting a divisor for an attribute and to set the number of instances of a mesh (Mesh.setInstances(n)).




