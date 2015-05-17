#include <assert.h>
#include "SurfaceGenerator.h"

SurfaceGenerator::SurfaceGenerator(int uSegments, int vSegments, bool closeU, bool closeV) :
_uSegments(uSegments), _vSegments(vSegments), _closeU(closeU), _closeV(closeV)
{
    // TODO: close flags not yet implemented; always assumes closed on both parameters.
    _uvVertex.resize(_uSegments * _vSegments);
    _uvNormal.resize(_uSegments * _vSegments, QVector3D(0, 0, 0));
    _divideCount.resize(_uSegments * _vSegments, 0);
}

// Return the packed buffer as a vector.
const std::vector<float>& SurfaceGenerator::generate()
{
    generateUVVertex();
    generateTrianglesAndUVs();
    generateNormals();

    // Pack data into a single buffer.  We have 8 floats per triangle (position, normal, UV)
    assert(_triangles.size() == _normals.size() && _triangles.size() == _uvs.size());
    assert(_triangles.size() % 3 == 0);

    _vertexCount = (int)_triangles.size();
    _buffer.resize(8*_vertexCount);

    auto itCoord = _buffer.begin();
    auto itNorm = itCoord + 3*_vertexCount;
    auto itUV = itNorm + 3*_vertexCount;

    for (auto i = 0; i < _triangles.size(); ++i) {
        assign(3, _triangles[i], itCoord);
        assign(3, _normals[i], itNorm);
        assign(2, _uvs[i], itUV);
    }

    _uvVertex.clear();
    _uvNormal.clear();
    _triangles.clear();
    _normals.clear();
    _uvs.clear();
    _divideCount.clear();

    return _buffer;
}

void SurfaceGenerator::generateUVVertex()
{
    for (int u = 0; u < _uSegments; ++u)
    for (int v = 0; v < _vSegments; ++v)
    {
        auto uv = UV(u, v);
        _uvVertex[VI(u, v)] = F(uv);
    }
}

void SurfaceGenerator::generateTrianglesAndUVs()
{
    for (int u = 0; u < _uSegments; ++u)
    for (int v = 0; v < _vSegments; ++v)
    {
        halfQuadVertex(u, v, 0);
        halfQuadVertex(u, v, 1);
    }
}

void SurfaceGenerator::generateNormals()
{
    assert(_uvNormal.size() == _divideCount.size());

    for (auto i = 0; i < _uvNormal.size(); ++i)
        _uvNormal[i] /= (float)_divideCount[i];

    for (int u = 0; u < _uSegments; ++u)
    for (int v = 0; v < _vSegments; ++v)
    {
        halfQuadNormal(u, v, 0);
        halfQuadNormal(u, v, 1);
    }
}

// Generate quad with LL vertex at (u,v).  Depending on h, either 0,1,2 or 0,2,3 triangle is generated.
void SurfaceGenerator::halfQuadVertex(int u, int v, int h)
{
    assert(h == 0 || h == 1);
    int u1 = (u+1) % _uSegments, v1 = (v+1) % _vSegments;
    int i[4] = { VI(u, v), VI(u1, v), VI(u1, v1), VI(u, v1) };
    QVector3D c[3] = { _uvVertex[i[0]], _uvVertex[i[1 + h]], _uvVertex[i[2 + h]] };
    QVector3D n = QVector3D::normal(c[0], c[1], c[2]);
    
    _triangles.push_back(c[0]); _uvs.push_back(UV(u, v));
    _triangles.push_back(c[1]); _uvs.push_back(UV(u1, !h ? v : v1));
    _triangles.push_back(c[2]); _uvs.push_back(UV(!h ? u : u1, v));

    _uvNormal[i[0]] += n;   ++_divideCount[i[0]];
    _uvNormal[i[1+h]] += n; ++_divideCount[i[1+h]];
    _uvNormal[i[2+h]] += n; ++_divideCount[i[2+h]];
}

void SurfaceGenerator::halfQuadNormal(int u, int v, int h)
{
    assert(h == 0 || h == 1);
    int u1 = (u + 1) % _uSegments, v1 = (v + 1) % _vSegments;
    int i[4] = { VI(u, v), VI(u1, v), VI(u1, v1), VI(u, v1) };

    _normals.push_back(_uvNormal[i[0]]);
    _normals.push_back(_uvNormal[i[1+h]]);
    _normals.push_back(_uvNormal[i[2+h]]);
}


