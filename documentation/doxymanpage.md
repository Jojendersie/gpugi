
General program structure
============================
The Program entry point is contained in main.cpp. It creates an instance of Application and calls the run function within a try catch block.

The Application class itself contains all main program modules and keeps the simulation running. The main loop is roughly separated in Update and Draw. Update processes all inputs, Draw updates the RendererSystem and the visible content of the OutputWindow.

The application can be started with a script (which is handed to ScriptProcessing) or blank, relying completely on command line input. Command line input will be forwarded to the ScriptProcessing module which mainly changes global config variables, defined in the GlobalConfig module.

At startup a single instance of RendererSystem is created. RendererSystem holds up to one Renderer and DebugRenderer.


RendererSystem
============================
The RendererSystem provides a framework for all other renderers.

Each call to RendererSystem::Draw is called iteration and usually increases the iteration count if no DebugRenderer is set (and not manipulated elsewhere).

It provides several buffer(views) which are available to all renderers:
* Scene data in texture buffers views (for binding indices see RendererSystem::TextureBufferBindings, GPU sided definition in "shader/scenedata.glsl")
  * triangles (Scene::GetTriangleBuffer)
  * vertex positions (Scene::GetVertexPositionBuffer)
  * vertex info (texcoord, normal, see Scene::GetVertexInfoBuffer)
  * basic hierarchy info (bounding volumes + child & escape pointer, see Scene::GetHierarchyBuffer)
  * initial light samples
* Uniform buffers (for binding indices see RendererSystem::UniformBufferBindings, GPU sided definition in "shader/globaluniforms.glsl")
  * global const (expected to change extremly rarely)
  * camera (expected to change only on camera movements)
  * per iteration (expected to change every iteration)

Glhelper allows to create uniform buffers from shader-meta-info. To create the mentioned UBOs, a special dummy shader is used to obtain the layouts automatically. Since the GLSL UBO layout is set to `shared` it is not necessary to alter the dummy shader on changes or additions to the UBOs (see also `shader/globaluniforms.glsl`).

The RendererSystem contains its own HDR (RGBA32F) "backbuffer" that can be obtained with RendererSystem::GetBackbuffer(). All rendering happens in either Renderer or DebugRenderer.

### InitialLightSamples ###
"InitialLightSamples" are changing random pointsamples on the scene`s light. Samples are recreated during RendererSystem::PerIterationBufferUpdate using LightTriangleSampler::GenerateRandomSamples. A renderer needs to specify how many initial light samples are needed using RendererSystem::SetNumInitialLightSamples. Currently the value is hardcoded for each individual renderer. Note that before this call no light samples are created.

Renderer
----------------------------
Renderers are set using RendererSystem::SetRenderer. It is not possible to set multiple Renderers at once. Setting a new Renderer will destroy the old one, reset the iteration count and clear the backbuffer. Each Renderer constructor is expected to expect a reference to the overlaying RendererSystem in the constructor.

### Type Overview ###
* Pathtracer
  * Simple pathtracer, useful for rendering ground truth images

* LightPathtracer
  * Sends rays from the light instead of camera. Each ray will perform a next event estimation to the camera.
  * Writing to pixels involves a spin-lock on the affected pixel (uses an additional image for locks).

* BidirectionalPathtracer
  * Bidirectional pathtracer using a lightcache as described by \cite bib:GPUSurvey2014
  * Multiple importance weights as in \cite bib:AntwerpenStreamingPathtracing
  * Light cache size is determined by a startup pass in BidirectionalPathtracer::CreateLightCacheWithCapacityEstimate

* HierarchyImportance
  * Obtains "importance" for all triangles using a pathtracer. For each path vertex all preceeding triangle-hits will increase their importance by the received radiance.
  * Propagation through the hierachy can be performed using HierarchyImportance::UpdateHierarchyNodeImportance
  * Hierachy information buffer (SSBO) is available as _shared pointer_ (for later use in other renderers) using HierarchyImportance::GetHierachyImportance. This means that the buffer survives a Renderer change, if you hold a copy of the shared_ptr somewhere else. Note however that switching to HierarchyImportance will always create a new buffer.
  
* Whitted Pathtracer
  * Variant of the pathtracer (adds the define STOP_ON_DIFFUSE_BOUNCE)
  * No indirect lighting or caustics -> no fireflies, strongly biased
  * 4x faster than Pathtracer and more like a debug renderer (due to bias)


### General Notes ###
* `SHOW_SPECIFIC_PATHLENGTH`
  * All existing Renderer provide the possibility to show only the results of a given pathlength, ignoring radiance obtained from all paths with different lengths. To enable this feature define `SHOW_SPECIFIC_PATHLENGTH x` in renderer.hpp, with `x` beeing the desired pathlength.

DebugRenderer
----------------------------
DebugRenderers are set using RendererSystem::SetDebugRenderer. The call only succeeds if there is set Renderer. Setting a new DebugRenderer or changing the Renderer will destroy the previous DebugRenderer. As long as a DebugRenderer is called, Renderer::Draw will redirect to the DebugRenderer. This will also suppress calls to Renderer::PerIterationBufferUpdate. To stop debug rendering and resume the normal renderer call RendererSystem::DisableDebugRenderer. This will destroy the DebugRenderer and switch back to the Renderer. Note that while switching DebugRendering does contain all Renderer relevant data, Camera movements may not.

### Type Overview ###
* RaytraceMeshInfo
  * Displays various scene information using a raytracer
  * Display type can (only) be set via GlobalConfig (use "help" command to list them)
  * Currently supported:
    * 0: World-Normal (color = n*.5 + .5) +
	* 1: Diffuse color
	* 2: Opacity color
	* 3: Reflection color
  
* HierarchyVisualization
  * Displays the scene hierachy as boxes (solid or wireframe)
  * By default, coloring is defined by heatmap from low hierachy levels (hot) to high hierarchy levels (cold)
  * If the active Renderer is of type HierarchyImportance, the coloring displays the importance of the node. Switiching to the HierarchyVisualization will then automatically call HierarchyImportance::UpdateHierarchyNodeImportance

Shaderfiles
----------------------------
File endings:
* glsl
  * used as include file only
* comp
  * compute shader
* frag
  * fragment shader
* vert
  * vertex shader

Overview of shader files. Configurations are done via #define macros (can be set via another shader file before #include or at runtime using gl::ShaderObject::AddShaderFromFile). Configuration options taged with [built-in] refer to a #define macro which is part of the shader and may be manually changed for reconfiguring before program start.

* `shader/`
  * `displayHDR.frag`
    * Outputs 2D texture, dividing colors by a given divisor (uniform var). Used by OutputWindow
  * `dummy.comp`
    * Dummy compute shader to obtain shared UBO infos. Used by RendererSystem::InitStandardUBOs
  * `globaluniforms.glsl`
    * Defines global uniform buffers. See RendererSystem::UniformBufferBindings
  * `helper.glsl`
    * Various general helper functions
  * `intersectiontests.glsl`
    * Functions for basic geometry intersection tests
  * `lightcache.glsl`
    * Defines lightcache layout and buffer (SSBO + shared UBO for meta info)
    * Contains also wrapper for multiple importance sampling heuristic (MISHeuristic, defaulting to balance heuristic)
    * Configuration options:
      * `SAVE_LIGHT_CACHE`: Write access to LightCache SSBO
      * `SAVE_LIGHT_CACHE_WARMUP`: (only valid with SAVE_LIGHT_CACHE) Additional SSBO for sum of light path length (SSBO contains a single integer).
  * `lighttracer.comp`
    * Compute shader for particle tracing. Used by LightPathtracer and BidirectionalPathtracer.
    * Configuration options:
      * `SAVE_LIGHT_CACHE`: Writes all particles, including initial samples, to the light cache for later use (in `pathtracer_bidir.comp`). Note that this will also reduce the maximal pathlength (`MAX_PATHLENGTH`).
      * `SHOW_SPECIFIC_PATHLENGTH`: See Renderer documentation.
      * [built-in] `MAX_PATHLENGTH`: Specifies the maximal length of a particle path.
  * `materials.glsl`
    * Provides BSDF sampling/evaluation and some other material related functions.
    * BSDF functions come in a normal and "Adjoint" version for particle tracing (see code comments)
    * Configuration options:
      * `SAMPLEBSDF_OUTPUT_PDF`: If set, the propability density at the sampled or evaluated point will also be returned.
	  * `STOP_ON_DIFFUSE_BOUNCE`: Whitted variant - manipulates throughput for BSDF and Sampling to stop on diffuse bounce and avoid fireflies
  * `pathtracer.comp`
    * Basic pathtracer, see Pathtracer class.
    * Configuration options:
      * `SHOW_SPECIFIC_PATHLENGTH`: See Renderer documentation.
      * [built-in] `MAX_PATHLENGTH`: Specifies the maximal length of a camera path.
      * [built-in] `NUM_LIGHT_SAMPLES`: How many light samples are evaluated for each ray hit.
  * `pathtracer_bidir.comp`
  	* Pathtracing part of the bidirectional pathtracer. See BidirectionalPathtracer class.
    * Configuration options:
      * `SHOW_SPECIFIC_PATHLENGTH`: See Renderer documentation.
      * [built-in] `MAX_PATHLENGTH`: Specifies the maximal length of a camera path.
      * [built-in] `MIN_CONNECTION_DISTSQ`: Light samples that are closer to camera samples than this value will be ignored.
  * `random.glsl`
    * Provides random value functions.
  * `scenedata.glsl`
    * Defines memory layout of the scene. See RendererSystem::TextureBufferBindings
  * `stdheader.glsl`
    * Combines various header needed for most raytracing based rendering.
    * Configuration options:
      * [built-in] `RAY_HIT_EPSILON`: Offset usually applied to ray hits for next trace
      * [built-in] `RAY_MAX`: Maximum length of a ray.
      * [built-in] `RUSSIAN_ROULETTE`: If defined, `pathtracer.comp`, `pathtracer_bidir.comp`, `lighttracer.comp` and `hierarchy/acqusition.comp` will use Russian Roulette to cancel rays. 
      * [built-in] `TRACERAY_DEBUG_VARS`: Defines debug variables for `traceray.glsl` (see `traceray.glsl` for more info)
      Otherwise always MAX_PATHLENGTH ray hits will be computed.
  * `traceray.glsl`
    * Traces a ray in the scene
    * Configuration options:
      * [built-in] `ANY_HIT`: Code defines TraceRayAnyHit, instead of TraceRay. (you can include this file multiple times!)
      * [built-in] `TRACERAY_DEBUG_VARS`: If defined a global variable called `numBoxesVisited`/`numTrianglesVisited` will be inkremented on each box/triangle test.
      * [built-in] `TRINORMAL_OUTPUT`: TraceRay function outputs the normal of the hit triangle.
      * [built-in] `TRIINDEX_OUTPUT`: TraceRay function outputs the index of the hit triangle.
* `debug/`
  * `hierarchybox.frag`
    * Fragment shader for HierarchyVisualization if not in HierarchyImportance mode.
    * Expects `hierarchybox.vert` as vertex shader.
  * `hierarchybox_importance.frag`
    * Fragment shader for HierarchyVisualization if in HierarchyImportance mode.
    * Expects `hierarchybox.vert` as vertex shader.
  * `hierarchybox.vert`
    * Vertex shader for HierarchyVisualization. Instancing of hierachy boxes.
  * `raytracemeshinfo.frag`
    * Fragment shader for RaytraceMeshInfo.
    * Expects `utils/screenTri.vert` as vertex shader.
* `hierarchy/`
  * `acquisition.comp`
    * HierarchyImportance's pathtracer to acquire per triangle importance.
  * `hierarchypropagation_init.comp`
    * Init pass of HierarchyImportance::UpdateHierarchyNodeImportance.
    * Each leaf node gathers importance of all its triangles.
  * `hierarchypropagation_nodes.comp`
    * Iteration pass of HierarchyImportance::UpdateHierarchyNodeImportance.
    * Each node gathers importance of all its childs.
  * `importanceubo.glsl`
    * Contains a uniform buffer with info about how the HierarchyImportanceBuffer is separated
  * `trianglevis.comp`
    * Raytracer that visualizes the importance per triangle.
* `utils/`
  * `screenTri.vert`
    * Screen aligned triangle. Usually used with gl::ScreenAlignedTriangle
  * `sqerror.frag`
    * Computes the per-pixel squared error between two color textures, where one has a variable divisor (UBO)
    * Used in TextureMSE


ScriptProcessing & GlobalConfig
==============================
GPUGI uses a simple command list script to make configuration at runtime or per file possible. Commands are parsed via ScriptProcessing. Console inputs are continously read in a separate thread (see ScriptProcessing::StartConsoleWindowThread and ScriptProcessing::StopConsoleWindowThread) and evaluated together with script commands during a ScriptProcessing::ProcessCommandQueue call.

With the exception of a few built-in commands, all commands will be passed to GlobalConfig. GlobalConfig is a namespace containing several functions to control a global variable pool. 
It is possible to use GlobalConfig like a global event-manager, since each global variable is a list of Variants (see GlobalConfig::ParameterType) with an arbitrary number of listeners. The size of these Variant-lists is user-defined and may even be empty to realize global functions like "screenshot" or "exit".

GlobalConfig is based on the Variant template class (see GlobalConfig::ParameterElementType). An instance of variant is guaranteed to be able to be converted to the provided template parameter. Note that it needs at least as much memory as the smallest given type (there might be additional padding).


Reserved/Built-In Commands
----------------------------
A script without any wait command will processed in a single call of ScriptProcessing::ProcessCommandQueue. To be able to run a Renderer in between use one of the following buildt-in wait commands:

* waitIterations [uint]
  * Stops script processing for the given number of update calls.
* waitSeconds [float]
  * Stops script processing until the given number of seconds has passed since the last ScriptProcessing::ProcessCommandQueue call.

To create custom commands that cancel the script processing register a command via ScriptProcessing::AddProcessingPauseCommand.

To show a list of all commands and their documention in the console use _help_.


TextureMSE
==============================
Class for computing the mean squared error of two images on the GPU. Reference image can be loaded from pfm. Application provides a binding to ScriptProcessing that makes it possible to wait for a Renderer to converge to a reference until a given MSE is reached.
