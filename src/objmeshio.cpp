/*
 *  Copyright (c) 2020 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
 */
#include "objmeshio.h"

#include <QFile>
#include <QIODevice>
#include <QTextStream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "version.h"

namespace ObjMeshIo {

bool loadWavefrontObj(const QString &path,
    std::vector<AutoRemesher::Vector3> &vertices,
    std::vector<std::vector<size_t>> &triangles,
    QString *errorOut)
{
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool loadSuccess = tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err,
        path.toUtf8().constData());
    if (!loadSuccess) {
        if (errorOut)
            *errorOut = QString::fromStdString(err.empty() ? warn : err);
        return false;
    }

    vertices.resize(attributes.vertices.size() / 3);
    for (size_t i = 0, j = 0; i < vertices.size(); ++i) {
        auto &dest = vertices[i];
        dest.setX(attributes.vertices[j++]);
        dest.setY(attributes.vertices[j++]);
        dest.setZ(attributes.vertices[j++]);
    }

    triangles.clear();
    for (const auto &shape : shapes) {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            triangles.push_back(std::vector<size_t> {
                (size_t)shape.mesh.indices[i + 0].vertex_index,
                (size_t)shape.mesh.indices[i + 1].vertex_index,
                (size_t)shape.mesh.indices[i + 2].vertex_index,
            });
        }
    }

    return true;
}

bool writeQuadObj(QIODevice &device,
    const std::vector<AutoRemesher::Vector3> &vertices,
    const std::vector<std::vector<size_t>> &quads,
    QString *errorOut)
{
    if (!device.isOpen()) {
        if (errorOut)
            *errorOut = QStringLiteral("Output device is not open.");
        return false;
    }

    QTextStream stream(&device);
    stream << "# " << APP_NAME << " " << APP_HUMAN_VER << Qt::endl;
    for (const auto &v : vertices)
        stream << "v " << v.x() << " " << v.y() << " " << v.z() << Qt::endl;
    for (const auto &face : quads) {
        stream << "f";
        for (size_t idx : face)
            stream << " " << (1 + idx);
        stream << Qt::endl;
    }

    return true;
}

}
