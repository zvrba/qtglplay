// The very first example from the red book, adapted to QT.

#pragma once
#ifndef PROJECTIVE_WIDGET_H
#define PROJECTIVE_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <complex>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class BoysGenerator
{
    QVector<glm::vec2> _uvs, _renderUVs;
    QVector<glm::vec3> _vertices, _normals, _renderTriangles, _renderNormals;
    QVector<unsigned short> _normalCounts;

    void generateTriangles(int uSegments, int vSegments);
    void generateNormals(int uSegments, int vSegments);
    void generateTriangle(int i0, int i1, int i2);

    using complex = std::complex<double>;
    static complex fromPolar(double r, double phi);
    static glm::vec3 bryant(complex z);


public:
    const QVector<glm::vec3> &getTriangles() const { return _renderTriangles; }
    const QVector<glm::vec3> &getNormals() const { return _renderNormals; }
    const QVector<glm::vec2> &getUVs() const { return _renderUVs; }

    void bryantsParametrization(int uSegments, int vSegments);
};

class ProjectiveWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    ProjectiveWidget(QWidget *parent = 0);
    ~ProjectiveWidget();

    QSize minimumSizeHint() const override { return QSize(320, 200); }
    QSize sizeHint() const override { return QSize(800, 600); }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    void setupGeometry();
    void setupProgram();
    void cleanup();


    QOpenGLVertexArrayObject _vertexArray;
    QOpenGLBuffer _vertexBuffer;
    QOpenGLShaderProgram _shaderProgram;
};

#endif