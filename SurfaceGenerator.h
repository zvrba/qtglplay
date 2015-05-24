#pragma once

#include <vector>
#include <QVector2D>
#include <QVector3D>

class SurfaceGenerator
{
    const int _uSegments, _vSegments;
    const bool _closeU, _closeV;

    // Temporary data.
    std::vector<QVector3D> _uvVertex, _uvNormal;
    // 3 elements per triangle
    std::vector<QVector3D> _triangles, _normals;
    std::vector<QVector2D> _uvs;
    std::vector<short> _divideCount;

    // Outputs
    std::vector<float> _buffer;
    int _vertexCount;

    int VI(int u, int v) const { return u * _vSegments + v; }
    
    void generateUVVertex();
    void generateTrianglesAndUVs();
    void generateNormals();
    void halfQuadVertex(int u, int v, int h);
    void halfQuadNormal(int u, int v, int h);
    
    template<typename T>
    static void assign(int n, T vec, std::vector<float>::iterator &it)
    {
        for (auto i = 0; i < n; ++i) *it++ = vec[i];
    }

protected:
    int getUSegmentCount() const { return _uSegments; }
    int getVSegmentCount() const { return _vSegments; }

    virtual QVector2D UV(int u, int v) const = 0;
    virtual QVector3D F(QVector2D uv) const = 0;

public:
    SurfaceGenerator(int uSegments, int vSegments, bool closeU, bool closeV);
    const std::vector<float> &generate();
    int getVertexCount() const { return _vertexCount; }
    int getTriangleCount() const { return _vertexCount / 3; }   // vertices are duplicated ATM
};

