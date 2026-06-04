# zoi

A software renderer to understand 3D graphics programming.

<video controls width="500">
  <source src="demo.mp4" type="video/mp4" />
</video>


## Features

- From scratch software rasterizer
- Top-left rule to avoid pixel overdraw
- Perspective correct interpolation
- Z-buffer
- MSAA 4x
- Textures + bilinear filtering
- Back-face culling
- Gamma correction + tone mapping
- OBJ parser (Very basic one)
- Camera system

## Partially implemented

- Flat shading + PBR (Cook-Torrance, Fresnel, NDF)
- Shadow buffer
- Mipmaps