// The very first example from the red book, adapted to QT.

#pragma once
#ifndef PROJECTIVE_WIDGET_H
#define PROJECTIVE_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

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
    void keyPressEvent(QKeyEvent*) override;

private:
    void setupGeometry();
    void loadProgram();
    void setupXform();
    void resetXform();
    void cleanup();

    static float radians(int angle) { return (float)(angle % 360) * 3.141593f / 180.0f; }

    // Object data & state. We have only a single object.
    int _vertexCount, _triangleCount;
    int _rx, _ry, _rz;  // Axis rotations in degrees.
    int _tz;            // Translation along z, multiplied by 10.
    int _znear;
    QMatrix4x4 _objectXform, _perspXform, _xform;

    QOpenGLVertexArrayObject _vao;
    QOpenGLBuffer _vbo;
    QOpenGLShaderProgram _program;
};

#endif