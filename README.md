# Simulant

**Development of Simulant has moved to [Gitlab](https://gitlab.com/simulant/simulant/)**

## What is it?

Simulant is my accidental hobby engine. It started while I was reverse-engineering a model format and wanted to quickly and easily manipulate meshes. Over the next couple of years I chipped away at it here and there, using it for various little toy projects, but it grew out of control without any focus. Over time it became a dumping ground for useful graphics-related code, and depended on more and more third party libraries. 

In 2014, I decided that it there were a lot of nice things in Simulant, but it was a mess, and so I developed my first game "Rocks & Spaceworms" (https://play.google.com/store/apps/details?id=uk.co.kazade.rocks) with it and ported the code to Android. This allowed me to start focusing on removing dependencies and structuring the codebase like a real game engine. 

Since then the focus has been to remove dependencies, build an easy-to-use API, and performance. The engine now is already very different to the one that powered Rocks & Spaceworms. My future plan is to port the engine to "retro" consoles like the Dreamcast, and also Windows, OSX, iOS, and perhaps even PSP or NDS. Although you could use this engine for modern graphics (the material system is very powerful), it's generally aimed at providing mid-2000s style game graphics. 

There are some glaring holes in functionality (LOD, billboards, shadows, projected textures) but you can already build some impressive things, and those features will arrive eventually, hopefully sooner rather than later!

## Features

 * Easy to use API, and getting simpler all the time!
 * Flexible rendering pipeline
 * Complex material scripting format
 * Loading of .obj models and the X-Wing vs Tie Fighter .opt format
 * Loading of MD2 animated models
 * Loading of Q2 BSP files (needs work)
 * Loading of heightmap terrains from image files
 * Advanced spatial hash partitioning and culling
 * Loading of JPG, PNG, TGA, WAL, PCX images and more!
 * Shortcut functions for loading 2D sprites, 2D backgrounds and 3D skyboxes
 * Simple scene graph functions
 * UI widgets (Button, Label, Progress Bar)
 * Basic rigid body physics using the Bounce library
 * Procedural functions for generating spheres, cubes, capsules, circles and rectangles
 * Functions for creating lights, multiple viewports and cameras

## Screenshots

![screenshot 1](/screenshots/screenshot1.png?raw=true)
![screenshot 2](/screenshots/screenshot2.png?raw=true)
![screenshot 3](/screenshots/screenshot3.png?raw=true)
![screenshot 4](/screenshots/screenshot4.png?raw=true)

## Roadmap / TODO

 * Finish the GL 1.x renderer
 * Switch the GL 2.x renderer with a GL 4.x one
 * Implement render-to-texture
 * Fix lightmaps in the Q2 bsp loader
 * Implement multiple LOD for meshes
 * Build in support for shadows
 * Improve Dreamcast support (depends on GL 1.x)
 * Restore Android support
 * Port to Windows, then iOS
 * Implement VBO management in the GL 2.x renderer for improved performance 

## Building

Compiling the code requires CMake. Currently there are the following external dependencies:

 - ZLIB
 - SDL2
 - OpenGL
 - TinyXML
 - OpenAL

To build:

```
    git submodule update --init 
    mkdir build && cd build
    cmake ..
    make
```

## How do I use it?

The easiest way to get started is to use simulant-tools which is a Python package (so requires Python and Pip installed):

```
pip install --user --upgrade git+git://github.com/kazade/simulant-tools.git
simulant start myproject
cd myproject
simulant run --rebuild
```

This will generate a template project, download pre-compiled Simulant libraries, and compile and run the application.

The advantage of using simulant-tools and sticking to that project structure is this:

```
simulant package dreamcast
```

Which would generate a .cdi image of your application for the SEGA Dreamcast.

Or

```
simulant run dreamcast
```

Which currently uses the lxdream emulator to run the .cdi image.

