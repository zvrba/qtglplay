#pragma once

#include <vector>
#include <QVector2D>
#include <QVector3D>

class SurfaceGenerator
{
    int _uSegments, _vSegments;
    bool _closeU, _closeV;

    // Temporary data.
    std::vector<QVector3D> _uvVertex, _uvNormal;
    
    // 3 elements per triangle
    std::vector<QVector3D> _triangles, _normals;
    std::vector<QVector2D> _uvs;
    std::vector<short> _divideCount;

    int VI(int u, int v) const { return u * _vSegments + v; }
    
    void generateUVVertex();
    void generateTrianglesAndUVs();
    void generateSmoothNormals();
    void generateFlatNormals();
    void halfQuadVertex(int u, int v, int h);
    void halfQuadSmoothNormal(int u, int v, int h);
    void halfQuadFlatNormal(int u, int v, int h);
    
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
    void generate(int uSegments, int vSegments, bool closeU, bool closeV);
    int uvIndex(int u, int v) { return VI(u, v); }
    const std::vector<QVector3D> &getTriangles() const { return _triangles; }
    const std::vector<QVector3D> &getNormals() const { return _normals; }
    const std::vector<QVector2D> &getUVs() const { return _uvs; }
};

// TODO! Move geometry generation into the vertex shader!
class ProjectiveGenerator : public SurfaceGenerator
{
protected:
    virtual QVector2D UV(int u, int v) const override
    {
        float uu = (float)u / getUSegmentCount();
        float vv = (float)v / getVSegmentCount();
        return QVector2D(uu, vv);
    }

    virtual QVector3D F(QVector2D uv) const override
    {
        static const float pi = 3.1416f;
        double u = uv.x() * 2 * pi, v = uv.y() * 2 * pi;
        double x = (1 + cos(v)) * cos(u);
        double y = (1 + cos(v)) * sin(u);
        double z = -tanh(u-pi) * sin(v);
        return QVector3D(x, y, z);
    }
};

