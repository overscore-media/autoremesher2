/*
 *  Copyright (c) 2020 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
 *
 *  Shared Wavefront OBJ load/save helpers for GUI and CLI.
 */
#ifndef AUTO_REMESHER_OBJ_MESH_IO_H
#define AUTO_REMESHER_OBJ_MESH_IO_H

#include <QString>
#include <vector>
#include <AutoRemesher/AutoRemesher>

class QIODevice;

namespace ObjMeshIo {

bool loadWavefrontObj(const QString &path,
    std::vector<AutoRemesher::Vector3> &vertices,
    std::vector<std::vector<size_t>> &triangles,
    QString *errorOut = nullptr);

bool writeQuadObj(QIODevice &device,
    const std::vector<AutoRemesher::Vector3> &vertices,
    const std::vector<std::vector<size_t>> &quads,
    QString *errorOut = nullptr);

}

#endif
