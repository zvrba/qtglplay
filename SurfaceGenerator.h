#pragma once
#ifndef SURFACEGENERATOR_H
#define SURFACEGENERATOR_H

#include <vector>

#define GLM_FORCE_SIZE_FUNC
#include <glm/glm.hpp>

class SurfaceGenerator
{
    const int _uSegments, _vSegments;
    const bool _closeU, _closeV;

    std::vector<glm::vec3> _uvVertex, _uvNormal;
    std::vector<glm::vec3> _triangles, _normals;
    std::vector<glm::vec2> _uvs;
    std::vector<short> _divideCount;

    int VI(int u, int v) const { return u * _vSegments + v; }

    void generateUVVertex();
    void generateTrianglesAndUVs();
    void generateNormals();
    void halfQuadVertex(int u, int v, int h);
    void halfQuadNormal(int u, int v, int h);

protected:
    int getUSegmentCount() const { return _uSegments; }
    int getVSegmentCount() const { return _vSegments; }

    virtual glm::vec2 UV(int u, int v) const = 0;
    virtual glm::vec3 F(glm::vec2 uv) const = 0;

public:
    SurfaceGenerator(int uSegments, int vSegments, bool closeU, bool closeV);
    const std::vector<glm::vec3>& getTriangles() const { return _triangles; }
    const std::vector<glm::vec3>& getNormals() const { return _normals; }
    const std::vector<glm::vec2>& getUVs() const { return _uvs; }
};

#endif