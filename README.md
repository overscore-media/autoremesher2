# AutoRemesher2

Automatic quad remeshing application built on [Geogram](http://alice.loria.fr/index.php/software/4-library/75-geogram.html), [libigl](https://github.com/libigl/libigl), [OpenVDB](https://www.openvdb.org/), [CGAL](https://www.cgal.org/), and [other libraries](ACKNOWLEDGEMENTS.html).

Fork of the excellent [autoremesher](https://github.com/huxingyi/autoremesher) by [Jeremy HU](https://github.com/huxingyi), with some dependency updates and UI enhancements.

Useful for remeshing models coming from photogrammetry, 3D scanners, and/or text/image-to-3D generators.

![Screenshot](/autoremesher2_screenshot.jpg)

See the releases tab for the latest build (Linux AppImage and Windows EXE)

## Building from Source

These steps assume a recent **Linux** system with Qt 5 and OpenVDB-related development packages installed.

```bash
git clone https://github.com/overscore-media/autoremesher2
cd autoremesher2
./scripts/bootstrap-submodules.sh
./scripts/build-bundled-deps.sh
qmake autoremesher.pro CONFIG+=release
make -j"$(nproc)"
./autoremesher
```

Full prerequisites (package names vary by distro), optional AppImage packaging, logo regeneration, and troubleshooting are described in **[BUILDING_LINUX.md](BUILDING_LINUX.md)**.

Windows builds (MSYS2/MinGW) are documented in **[BUILDING_WINDOWS.md](BUILDING_WINDOWS.md)**.

## Acknowledgements

See [ACKNOWLEDGEMENTS.html](ACKNOWLEDGEMENTS.html).

## Roadmap

The motivation behind this project was to update Autoremesher so it would be more stable on my system, and I was able to add some small quality-of-life features in the process. I don't see myself revisiting this project for a while, since it's working for my needs at the moment. However, in case I do end up revisiting it sooner, or if anyone ends up finding this fork useful, here are some ideas I have for future improvements.

Autoremesher v1 included support for macOS; it would be nice to re-enable support for macOS eventually. The main issue is the complexity of the build process.

Additional Linux distribution methods would also be ideal for portability (.deb, .rpm, flatpak, Arch PKGBUILD, etc.).

Support for importing/exporting other file types, especially GLTF/GLB, STL, and PLY would also be worth working on.

There are also some limitations with this software, specifically as it relates to models with a high number of vertices. I'm not sure where the cut-off point is, but there's definitely a limit and it may be somewhat restrictive. Ideally if a reasonable upper limit could be found, some kind of automated decimation could be run first if a model is detected as being over the limit. For the time being, that's easy enough to do in Blender/MeshLab as needed.

---

If you have ideas for additional features/improvements, feel free to mention them in an Issue, or create a Pull Request.