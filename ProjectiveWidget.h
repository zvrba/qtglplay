#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_3_3_core>
#include <QMatrix4x4>
#include "SurfaceGenerator.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class ProjectiveWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    ProjectiveWidget(QWidget *parent = 0);
    ~ProjectiveWidget();

    QSize minimumSizeHint() const override { return QSize(320, 200); }
    QSize sizeHint() const override { return QSize(800, 600); }

public slots:
    void setSegmentCount(int count);
    void setCameraU(int u);
    void setCameraV(int v);
    void setCameraHeight(int height);
    //void setCameraFOV(const QString &fov);
    void compileShaders();
    void cleanup();

signals:
    void cameraSurfacePositionChanged(const QVector3D newPosition);
    void cameraProjectionTargetChanged(const QVector3D newProjectionTarget);
    void compilationDone(const QString &msg);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void keyPressEvent(QKeyEvent *ev) override;

private:
    void loadProgram();
    void setupGeometry();
    void setupTexture();
    void setupCamera();

    // Camera position & orientation.
    int _segmentCount;
    int _vpWidth, _vpHeight;                        // viewport
    float _cameraU, _cameraV, _cameraHeading;       // camera position & movement direction on the surface
    float _cameraHeight, _cameraTilt, _cameraFOV;   // how high above camera is & up/down tilt

    // OpenGL stuff.
    ProjectiveGenerator _shapeData;
    int _triangleCount;
    QMatrix4x4 _xform;

    QOpenGLFunctions_3_3_Core *G;
    GLuint _vao, _vbo[3], _tex;
    GLint _vertex_position_i, _vertex_normal_i, _vertex_uv_i, _vmp_i, _tex_i;
    QOpenGLShaderProgram _program;
};

