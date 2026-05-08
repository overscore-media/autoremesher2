QT += core widgets opengl
CONFIG += release
CONFIG(release, debug|release) DEFINES += NDEBUG
DEFINES += AUTO_REMESHER_DEBUG
DEFINES += QT_MESSAGELOGCONTEXT
RESOURCES += resources.qrc

CONFIG += object_parallel_to_source

CONFIG(debug, debug|release) OBJECTS_DIR=build
CONFIG(release, debug|release) OBJECTS_DIR=build
CONFIG(debug, debug|release) MOC_DIR=build
CONFIG(release, debug|release) MOC_DIR=build

isEmpty(HUMAN_VERSION) {
	HUMAN_VERSION = "2.0.0"
}
isEmpty(VERSION) {
	VERSION = 2.0.0.0
}

unix {
    PROJECT_PLATFORM = Linux
    ICON = autoremesher.png
}

QMAKE_TARGET_PRODUCT = AutoRemesher
QMAKE_TARGET_DESCRIPTION = "AutoRemesher is a cross-platform open-source automatic quad remeshing software"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2026 OverScore Media"

DEFINES += "PROJECT_DEFINED_APP_NAME=\"\\\"$$QMAKE_TARGET_PRODUCT\\\"\""
DEFINES += "PROJECT_DEFINED_APP_VER=\"\\\"$$VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HUMAN_VER=\"\\\"$$HUMAN_VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_PLATFORM=\"\\\"$$PROJECT_PLATFORM\\\"\""

CONFIG += c++17

# GCC defines "linux" / "unix" as macros; QtAwesome's fa::linux icon name breaks compilation.
linux {
    QMAKE_CXXFLAGS += -Ulinux -Uunix
}

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

# Geogram expects GEOGRAM_VERSION when not using CMake (see src/lib/geogram/basic/common.cpp)
DEFINES += GEOGRAM_VERSION=\\\"1.9.9\\\"
# Use bundled stb / dependencies vendored under geogram/third_party (see image_serializer_stb.cpp)
DEFINES += GEOGRAM_USE_BUILTIN_DEPS

include(thirdparty/QtAwesome/QtAwesome/QtAwesome.pri)

INCLUDEPATH += thirdparty/QtWaitingSpinner

SOURCES += thirdparty/QtWaitingSpinner/waitingspinnerwidget.cpp
HEADERS += thirdparty/QtWaitingSpinner/waitingspinnerwidget.h

INCLUDEPATH += thirdparty/libigl/include
INCLUDEPATH += thirdparty/eigen

INCLUDEPATH += include

SOURCES += src/main.cpp

SOURCES += src/objmeshio.cpp
HEADERS += src/objmeshio.h

SOURCES += src/remeshcli.cpp
HEADERS += src/remeshcli.h

SOURCES += src/logbrowser.cpp
HEADERS += src/logbrowser.h

SOURCES += src/logbrowserdialog.cpp
HEADERS += src/logbrowserdialog.h

SOURCES += src/spinnableawesomebutton.cpp
HEADERS += src/spinnableawesomebutton.h

SOURCES += src/util.cpp
HEADERS += src/util.h

SOURCES += src/mainwindow.cpp
HEADERS += src/mainwindow.h

SOURCES += src/settingsdialog.cpp
HEADERS += src/settingsdialog.h

SOURCES += src/aboutwidget.cpp
HEADERS += src/aboutwidget.h

SOURCES += src/howtousewidget.cpp
HEADERS += src/howtousewidget.h

SOURCES += src/theme.cpp
HEADERS += src/theme.h

SOURCES += src/graphicscontainerwidget.cpp
HEADERS += src/graphicscontainerwidget.h

SOURCES += src/graphicswidget.cpp
HEADERS += src/graphicswidget.h

SOURCES += src/pbrshadermesh.cpp
HEADERS += src/pbrshadermesh.h

SOURCES += src/pbrshadermeshbinder.cpp
HEADERS += src/pbrshadermeshbinder.h

SOURCES += src/pbrshaderprogram.cpp
HEADERS += src/pbrshaderprogram.h

HEADERS += src/pbrshadervertex.h

SOURCES += src/pbrshaderwidget.cpp
HEADERS += src/pbrshaderwidget.h

SOURCES += src/rendermeshgenerator.cpp
HEADERS += src/rendermeshgenerator.h

SOURCES += src/quadmeshgenerator.cpp
HEADERS += src/quadmeshgenerator.h

SOURCES += src/ddsfile.cpp
HEADERS += src/ddsfile.h

SOURCES += src/preferences.cpp
HEADERS += src/preferences.h

SOURCES += src/floatnumberwidget.cpp
HEADERS += src/floatnumberwidget.h

SOURCES += src/AutoRemesher/autoremesher.cpp
HEADERS += src/AutoRemesher/autoremesher.h

SOURCES += src/AutoRemesher/isotropicremesher.cpp
HEADERS += src/AutoRemesher/isotropicremesher.h

SOURCES += src/AutoRemesher/vdbremesher.cpp
HEADERS += src/AutoRemesher/vdbremesher.h

SOURCES += src/AutoRemesher/parameterizer.cpp
HEADERS += src/AutoRemesher/parameterizer.h

SOURCES += src/AutoRemesher/quadextractor.cpp
HEADERS += src/AutoRemesher/quadextractor.h

SOURCES += src/AutoRemesher/positionkey.cpp
HEADERS += src/AutoRemesher/positionkey.h

SOURCES += src/AutoRemesher/meshseparator.cpp
HEADERS += src/AutoRemesher/meshseparator.h

SOURCES += src/AutoRemesher/relativeheight.cpp
HEADERS += src/AutoRemesher/relativeheight.h

# Geogram 1.9.9 (git submodule BrunoLevy/geogram tag v1.9.9 + recursive submodules)
GEOGRAM_SRC = thirdparty/geogram/src/lib
# Exploragram is a separate submodule (upstream ships it only if present under src/lib/exploragram)
EXPLORAGRAM_SRC = thirdparty/exploragram
# exploragram sources use includes like <exploragram/hexdom/...> (parent dir on include path is thirdparty/)
INCLUDEPATH += thirdparty

# Bundled zlib + oneTBB from scripts/build-bundled-deps.sh → thirdparty/.artifacts/
AUTOREMESHER_BUNDLE_STAMP = $$PWD/thirdparty/.artifacts/.stamp
AUTOREMESHER_BUNDLE_ROOT = $$PWD/thirdparty/.artifacts

unix:!macx {
    # Headers for app code (tbb::*, zlib if needed)
    INCLUDEPATH += $$AUTOREMESHER_BUNDLE_ROOT/onetbb/include
    INCLUDEPATH += $$AUTOREMESHER_BUNDLE_ROOT/zlib/include
    QMAKE_RPATHDIR += $$AUTOREMESHER_BUNDLE_ROOT/onetbb/lib
    LIBS += -L$$AUTOREMESHER_BUNDLE_ROOT/onetbb/lib -ltbb
    LIBS += -L$$AUTOREMESHER_BUNDLE_ROOT/zlib/lib -lz

    INCLUDEPATH += /usr/include/OpenEXR
    INCLUDEPATH += /usr/include/openvdb
    CONFIG += link_pkgconfig
    PKGCONFIG += OpenEXR
    INCLUDEPATH += /usr/include/openvdb
    LIBS += -lopenvdb -lgmp -lmpfr -ldl
}

win32 {
    INCLUDEPATH += /mingw64/include
    INCLUDEPATH += /mingw64/include/oneapi
    LIBS += -ltbb12
    LIBS += -L/mingw64/lib -lz

    INCLUDEPATH += /mingw64/include/OpenEXR
    INCLUDEPATH += /mingw64/include
    LIBS += -lopenvdb -lImath -lgmp -lmpfr -lopengl32
}

INCLUDEPATH += $$GEOGRAM_SRC
INCLUDEPATH += $$GEOGRAM_SRC/geogram/third_party/libMeshb/sources
INCLUDEPATH += $$GEOGRAM_SRC/geogram/third_party/rply
INCLUDEPATH += $$GEOGRAM_SRC/geogram/third_party/OpenNL
INCLUDEPATH += $$GEOGRAM_SRC/geogram/third_party
INCLUDEPATH += $$GEOGRAM_SRC/geogram/third_party/amgcl
INCLUDEPATH += $$GEOGRAM_SRC/geogram/third_party/stb_image

SOURCES += src/geogram_report_progress_bridge.cpp

SOURCES += $$GEOGRAM_SRC/geogram/basic/algorithm.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/algorithm.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/command_line.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/command_line.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/environment.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/environment.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/geometry.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/geometry.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/packed_arrays.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/packed_arrays.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/progress.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/progress.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/assert.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/assert.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/command_line_args.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/command_line_args.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/factory.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/factory.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/line_stream.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/line_stream.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/process.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/process.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/quaternion.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/quaternion.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/attributes.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/attributes.h

SOURCES += $$GEOGRAM_SRC/geogram/basic/common.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/common.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/file_system.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/file_system.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/logger.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/logger.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/process_unix.cpp
SOURCES += $$GEOGRAM_SRC/geogram/basic/stopwatch.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/stopwatch.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/b_stream.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/b_stream.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/counted.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/counted.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/geofile.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/geofile.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/numeric.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/numeric.h
SOURCES += $$GEOGRAM_SRC/geogram/basic/process_win.cpp
SOURCES += $$GEOGRAM_SRC/geogram/basic/string.cpp
HEADERS += $$GEOGRAM_SRC/geogram/basic/string.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/smart_pointer.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/matrix.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/process_private.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/argused.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/memory.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/psm.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/thread_sync.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/geometry_nd.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/vecg.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/permutation.h
HEADERS += $$GEOGRAM_SRC/geogram/basic/range.h

SOURCES += $$GEOGRAM_SRC/geogram/delaunay/delaunay.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/delaunay.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/delaunay_3d.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/delaunay_3d.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/delaunay_tetgen.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/delaunay_tetgen.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/LFS.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/LFS.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/periodic.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/periodic.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/delaunay_2d.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/delaunay_2d.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/delaunay_nn.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/delaunay_nn.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/delaunay_triangle.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/delaunay_triangle.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/parallel_delaunay_3d.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/parallel_delaunay_3d.h
SOURCES += $$GEOGRAM_SRC/geogram/delaunay/periodic_delaunay_3d.cpp
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/periodic_delaunay_3d.h
HEADERS += $$GEOGRAM_SRC/geogram/delaunay/cavity.h

SOURCES += $$GEOGRAM_SRC/geogram/bibliography/bibliography.cpp
HEADERS += $$GEOGRAM_SRC/geogram/bibliography/bibliography.h
SOURCES += $$GEOGRAM_SRC/geogram/bibliography/embedded_references.cpp

SOURCES += $$GEOGRAM_SRC/geogram/parameterization/mesh_global_param.cpp
HEADERS += $$GEOGRAM_SRC/geogram/parameterization/mesh_global_param.h

SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_AABB.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_AABB.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_frame_field.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_frame_field.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_fill_holes.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_fill_holes.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_geometry.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_geometry.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_halfedges.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_halfedges.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_io.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_io.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_topology.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_topology.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_partition.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_partition.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_preprocessing.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_preprocessing.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_reorder.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_reorder.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh_repair.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh_repair.h
SOURCES += $$GEOGRAM_SRC/geogram/mesh/mesh.cpp
HEADERS += $$GEOGRAM_SRC/geogram/mesh/mesh.h

SOURCES += $$GEOGRAM_SRC/geogram/points/co3ne.cpp
HEADERS += $$GEOGRAM_SRC/geogram/points/co3ne.h
SOURCES += $$GEOGRAM_SRC/geogram/points/colocate.cpp
HEADERS += $$GEOGRAM_SRC/geogram/points/colocate.h
SOURCES += $$GEOGRAM_SRC/geogram/points/kd_tree.cpp
HEADERS += $$GEOGRAM_SRC/geogram/points/kd_tree.h
SOURCES += $$GEOGRAM_SRC/geogram/points/nn_search.cpp
HEADERS += $$GEOGRAM_SRC/geogram/points/nn_search.h
SOURCES += $$GEOGRAM_SRC/geogram/points/principal_axes.cpp
HEADERS += $$GEOGRAM_SRC/geogram/points/principal_axes.h

SOURCES += $$GEOGRAM_SRC/geogram/numerics/expansion_nt.cpp
HEADERS += $$GEOGRAM_SRC/geogram/numerics/expansion_nt.h
SOURCES += $$GEOGRAM_SRC/geogram/numerics/lbfgs_optimizers.cpp
HEADERS += $$GEOGRAM_SRC/geogram/numerics/lbfgs_optimizers.h
SOURCES += $$GEOGRAM_SRC/geogram/numerics/matrix_util.cpp
HEADERS += $$GEOGRAM_SRC/geogram/numerics/matrix_util.h
SOURCES += $$GEOGRAM_SRC/geogram/numerics/multi_precision.cpp
HEADERS += $$GEOGRAM_SRC/geogram/numerics/multi_precision.h
SOURCES += $$GEOGRAM_SRC/geogram/numerics/optimizer.cpp
HEADERS += $$GEOGRAM_SRC/geogram/numerics/optimizer.h
SOURCES += $$GEOGRAM_SRC/geogram/numerics/predicates.cpp
HEADERS += $$GEOGRAM_SRC/geogram/numerics/predicates.h

HEADERS += $$GEOGRAM_SRC/geogram/NL/nl.h
HEADERS += $$GEOGRAM_SRC/geogram/NL/nl_iterative_solvers.h
HEADERS += $$GEOGRAM_SRC/geogram/NL/nl_blas.h
HEADERS += $$GEOGRAM_SRC/geogram/NL/nl_matrix.h
HEADERS += $$GEOGRAM_SRC/geogram/NL/nl_preconditioners.h
HEADERS += $$GEOGRAM_SRC/geogram/NL/nl_context.h
HEADERS += $$GEOGRAM_SRC/geogram/NL/nl_ext.h

SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_api.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_arpack.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_blas.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_cholmod.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_context.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_cuda.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_iterative_solvers.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_matrix.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_mkl.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_os.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_preconditioners.c
SOURCES += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_superlu.c
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_arpack.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_cholmod.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_cuda.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_mkl.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_superlu.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_private.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/OpenNL/nl_64.h

HEADERS += $$GEOGRAM_SRC/geogram/image/image_library.h

SOURCES += $$GEOGRAM_SRC/geogram/image/colormap.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/colormap.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image_library.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image_library.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image_serializer.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image_serializer.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image_serializer_stb.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image_serializer_stb.h
SOURCES += $$GEOGRAM_SRC/geogram/image/morpho_math.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/morpho_math.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image_rasterizer.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image_rasterizer.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image_serializer_pgm.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image_serializer_pgm.h
SOURCES += $$GEOGRAM_SRC/geogram/image/image_serializer_xpm.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/image_serializer_xpm.h
SOURCES += $$GEOGRAM_SRC/geogram/image/color.cpp
HEADERS += $$GEOGRAM_SRC/geogram/image/color.h

SOURCES += $$GEOGRAM_SRC/geogram/voronoi/convex_cell.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/convex_cell.h
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD_cell.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD_cell.h
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD.h
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD_polygon.h
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD_vertex.h
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD_utils.h
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/integration_simplex.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/integration_simplex.h
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/RVD_callback.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/RVD_callback.h
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/CVT.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/CVT.h
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/generic_RVD_polygon.cpp
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/RVD.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/RVD.h
SOURCES += $$GEOGRAM_SRC/geogram/voronoi/RVD_mesh_builder.cpp
HEADERS += $$GEOGRAM_SRC/geogram/voronoi/RVD_mesh_builder.h

SOURCES += $$GEOGRAM_SRC/geogram/third_party/libMeshb/sources/libmeshb7.c
HEADERS += $$GEOGRAM_SRC/geogram/third_party/libMeshb/sources/libmeshb7.h

SOURCES += $$GEOGRAM_SRC/geogram/third_party/rply/rply.c
HEADERS += $$GEOGRAM_SRC/geogram/third_party/rply/rply.h
HEADERS += $$GEOGRAM_SRC/geogram/third_party/rply/rplyfile.h

SOURCES += $$EXPLORAGRAM_SRC/hexdom/basic.cpp
HEADERS += $$EXPLORAGRAM_SRC/hexdom/basic.h
SOURCES += $$EXPLORAGRAM_SRC/hexdom/frame.cpp
HEADERS += $$EXPLORAGRAM_SRC/hexdom/frame.h
SOURCES += $$EXPLORAGRAM_SRC/hexdom/spherical_harmonics_l4.cpp
HEADERS += $$EXPLORAGRAM_SRC/hexdom/spherical_harmonics_l4.h
SOURCES += $$EXPLORAGRAM_SRC/hexdom/polygon.cpp
HEADERS += $$EXPLORAGRAM_SRC/hexdom/polygon.h
SOURCES += $$EXPLORAGRAM_SRC/hexdom/quad_cover.cpp
HEADERS += $$EXPLORAGRAM_SRC/hexdom/quad_cover.h

target.path = ./
INSTALLS += target