---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Graphics API: SDL3 GPU

## Context and Problem Statement

The new renderer needs a graphics API that works on Linux, Windows and macOS. No single
native API does: Vulkan is native on Linux and Windows, Direct3D 12 on Windows, Metal on
macOS, and OpenGL is frozen at 4.1 and deprecated on macOS. The feature scope is a
rasterised HDR, shadow and PBR pipeline without ray tracing
([ADR-0008](0008-renderer-feature-scope.md)). Which API does the renderer's back end
talk to?

## Decision Drivers

* First-class results on all three platforms, including Apple Silicon.
* Covers the feature scope: floating-point render targets, depth textures with
  comparison sampling, cube and array textures, MSAA, compute shaders, block-compressed
  textures, HDR output.
* Small dependency and build footprint; the project is meant to be clean.
* Amount of code between "nothing" and "the first lit, textured world".
* Debuggability with standard tools.
* The choice must still be alive in ten years.

## Considered Options

* SDL3 GPU API
* Vulkan directly, with a Vulkan-on-Metal layer on macOS
* WebGPU native (Dawn or wgpu-native)
* OpenGL 4.x core profile
* A rendering middleware such as bgfx

## Decision Outcome

Chosen option: "SDL3 GPU API", because it is the only option that gives native back ends
on all three platforms (Vulkan, Direct3D 12, Metal) without adding a dependency, since
SDL3 is already the platform layer ([ADR-0005](0005-platform-layer-sdl3.md)); because
its feature set covers the whole scope; and because the features it deliberately leaves
out are ones this project has ruled out.

The decision is accepted **subject to confirmation by a spike** at the start of
milestone M5 (see Confirmation). Direct Vulkan is the fallback.

Containment rule, whichever API is used: GPU API calls exist only in the back-end
directory of the new renderer. No custom abstraction layer is built on top; the
containment is what bounds the cost of changing the API.

### Consequences

* Good, because no additional library, loader, memory allocator or translation layer is
  needed on any platform.
* Good, because macOS uses Metal directly rather than a Vulkan translation layer.
* Good, because the API is small; resource cycling for per-frame dynamic geometry is
  built in.
* Bad, because shaders must be delivered in a different format per back end (SPIR-V,
  DXIL or DXBC, MSL or metallib). A build-time shader cross-compilation step is needed;
  the toolchain is its own decision at M5. Using only the Vulkan back end on Windows at
  first would reduce this to two formats.
* Bad, because there is no bindless resource model. Materials bind their textures per
  draw; lightmaps go into array textures or atlases. Acceptable at this game's scene
  complexity.
* Bad, because resource slot counts are fixed and low (for example four uniform buffer
  slots per shader stage), and the API does not expose hardware limits. Shaders must be
  designed within these.
* Bad, because the API is young (stable since SDL 3.2, January 2025) and the project
  would be among its more demanding users.
* Bad, because if the project ever wanted ray tracing after all, this choice would have
  to be reversed. The SDL maintainers state they have no near-term plans for ray tracing
  or mesh shaders.

### Confirmation

A time-boxed spike at the start of M5, on the real engine, must show on Linux, and on
Windows and macOS on real machines:

1. A retail map's world geometry drawn from static buffers with lightmaps, through
   shader variants generated from the shader scripts.
2. One shadow-mapped dynamic light using a depth target and comparison sampling.
3. Rendering into a floating-point target with tone mapping to the swap chain.
4. Per-frame streaming geometry (the 2D and effects path) without stalls.
5. One shader source compiled to every back end's format by the build.
6. A frame capture inspected in RenderDoc (Linux or Windows) and in Xcode's Metal
   debugger (macOS).

If any of these fails for a reason inherent to the API, this record is superseded by one
choosing direct Vulkan.

## Pros and Cons of the Options

### SDL3 GPU API

* Good, because native Vulkan, Direct3D 12 and Metal back ends in a library already used.
* Good, because the installed header (SDL 3.4) confirms: compute pipelines, storage
  buffers and textures, cube-array and 2D-array textures, `R16G16B16A16_FLOAT` and
  `R11G11B10_UFLOAT` targets, BC6H/BC7 and ASTC, `D32_FLOAT` depth, comparison samplers,
  anisotropy, MSAA sample counts, indirect draws, depth bias, HDR swap chain
  compositions, debug groups.
* Neutral, because no geometry or tessellation shaders; the renderer does not need them.
* Bad, because no bindless, fixed slot counts, per-back-end shader formats, young API.

### Vulkan directly

* Good, because the highest feature ceiling and the best tooling (validation layers,
  RenderDoc); one shader format (SPIR-V).
* Good, because the macOS story is now solid: MoltenVK supports Vulkan 1.4, and the
  Mesa-based KosmicKrisp driver is reported as Vulkan 1.4 conformant on Apple Silicon.
* Bad, because macOS still runs through a translation layer that has to be shipped with
  the game; MoltenVK is not a fully conformant implementation, and KosmicKrisp is Apple
  Silicon only.
* Bad, because it is by far the most code before the first pixel: device and swap chain
  management, memory allocation, descriptor management, synchronisation. Additional
  libraries (a loader, an allocator) are the norm.
* Bad, because most of its extra ceiling is in features this project has excluded.

### WebGPU native

* Good, because a modern, safe API with one shading language and all three back ends.
* Bad, because the implementations are heavy dependencies: Dawn is a very large build;
  wgpu-native needs a Rust toolchain and has lagged the standard C header.
* Bad, because the feature set is no larger than SDL3 GPU's, for a much larger
  footprint.

### OpenGL 4.x core profile

* Good, because closest to the existing renderer.
* Bad, because macOS stops at 4.1 with no compute shaders and is deprecated; ruled out
  by the platform requirement.

### Middleware (bgfx and similar)

* Good, because mature and broad back-end support.
* Bad, because it brings its own shader dialect, build tooling and rendering model, and
  offers nothing over SDL3 GPU that this project needs.

## More Information

Evidence gathered on 2026-11-23:

* SDL3 GPU capabilities were checked against the installed header,
  `SDL3/SDL_gpu.h` from SDL 3.4.
* SDL's own positioning: the GPU API targets widely supported features and portability,
  and is "probably not for you" if cutting-edge features are needed.
  <https://wiki.libsdl.org/SDL3/CategoryGPU>
* MoltenVK added Vulkan 1.4 support in v1.4.0.
  <https://github.com/KhronosGroup/MoltenVK/releases>
* KosmicKrisp reported conformant to Vulkan 1.4 on Apple Silicon (October 2026).
  <https://www.phoronix.com/news/KosmicKrisp-Vulkan-1.4-2026>
* LunarG on the state of Vulkan on Apple platforms, including MoltenVK's conformance
  status (January 2026). <https://www.lunarg.com/?p=4048>
* The WebGPU native header project notes that wgpu-native does not yet implement the
  stable header (date of that statement not verified).
  <https://github.com/webgpu-native/webgpu-headers>

Not verified here: SDL3 GPU behaviour on Windows and macOS hardware, and the current
state of SDL's shader cross-compilation tooling. Both are what the spike is for.
