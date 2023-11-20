
# TODO

Just a todo list of various things I need to do, in no particular order

### Renderer

- Per Frame Upload Heap
  - For Per-Frame Data and Geometry/Texture Uploads
- First Pass RenderMesh System
- First Pass RenderMaterial System
- Upload Queue and defer resource rendering until fully loaded
- Full Texture Implementation
  - Mip Map RenderPass
  - Might as well do the Cube Map generation RenderPass
- First Pass RenderTexture System
- ImGUI RenderPass!
  - Editor Implementation too
- Indirect (First Attempt)

### Engine-y Stuff

- Input system
- First Pass Geometry, Material, and Texture System
- First Pass (De)Serialization
- "project" files, mostly a way for me to have asset data separate from the engine
- File Tracking and File Hotreloading
  - Perhaps shader recompilation too
- Texture Loading
  - At first just use stb
  - Look into DDS
- glTF loading
- Scene structure serialization
