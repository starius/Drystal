Drystal
=======

About
-----

Drystal is an opensource game engine.

Games are programmed in Lua and can be exported to Javascript seamlessly.

Author
------

Jérémy Anger

Mailing list
------------

https://groups.google.com/forum/#!forum/drystal

To subscribe, send an e-mail to:

    drystal+subscribe@googlegroups.com

To unsubscribe, send an e-mail to:

    drystal+unsubscribe@googlegroups.com

To follow via RSS:

    https://groups.google.com/forum/feed/drystal/topics/rss.xml

To follow via ATOM:

    https://groups.google.com/forum/feed/drystal/topics/atom.xml

License
-------

LGPLv3 for all Drystal code

The external/ folder contains third-party libraries:

- stb\_vorbis.c and stb\_truetype.h which are in the public domain
- lua/\* which is under the MIT License
- box2d/* which is under the zlib License
- lua-cjson/\* which is under the MIT License
- wavloader.c which is in the public domain
- miniz.c which is in the public domain
- libpng/\* which is under the libpng License
- zlib/\* which is under the zlib License

Repository
----------

http://github.com/kidanger/drystal/

Documentation
-------------

http://drystal.github.io/

Requirements
------------

- SDL2
- OpenAL
- OpenGL
- libpng >= 1.6

Build:

- CMake
- C++ compiler (clang, gcc,...)
- make or ninja
- patch

Test coverage:

- gcov
- lcov

Documentation:

- sphinx
- pip

Web build:

- Emscripten

First time
----------

The first time you cloned Drystal you need to initialize the submodules:

    git submodule update --init --recursive

Native build of Drystal
-----------------------

    mkdir build-native-release
    cd build-native-release
    cmake .. -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release
    make

You can now find the executable in the src/ directory

    src/drystal --help

Web build of Drystal
--------------------

In order to compile Drystal to javascript you will need Emscripten.

    mkdir build-web-release
    cd build-web-release
    cmake .. -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/Emscripten.cmake -DEMSCRIPTEN_ROOT_PATH=/usr/lib/emscripten -DBUILD_LIVECODING=NO
    make

You will find the final javascript code (there will be two files drystal.js
and drystal.mem.js) in the src/ directory.

Build options
-------------

You can configure the build to fit to your needs, e.g. you do not need
the physics engine, you need to add -DBUILD_PHYSICS=OFF

Here is the list of options:

- BUILD_MANPAGES

    Default to ON

- BUILD_ENABLE_COVERAGE

    Default to OFF

- BUILD_LIVECODING

    Default to ON

- BUILD_PHYSICS, BUILD_PARTICLE, BUILD_WEB, BUILD_FONT, BUILD_AUDIO,
  BUILD_STORAGE, BUILD_GRAPHICS, BUILD_UTILS

    Default to ON

For the web build, removing parts of Drystal that you do not use decrease
the size of the final javascript code which helps loading the page of the
game faster.

Documentation build
-------------------

First, you need sphinx and pip (python2) then, as root, run:

    pip install sphinx_rtd_theme

If you want to build the documentation in HTML go to doc/ and run:

    make html
