#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_3_3_core>
#include <QMatrix4x4>

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
    void setCameraHeight(float height);
    void compileShaders();
    void cleanup();

signals:
    void compilationDone(const QString &msg);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    void loadProgram();
    void setupGeometry();
    void setupTexture();
    void setupCamera();

    // Camera position & orientation.
    int _segmentCount;
    int _cameraU, _cameraV;
    float _cameraHeight;

    // OpenGL stuff.
    int _vertexCount, _triangleCount;
    QMatrix4x4 _cameraXform, _xform;

    QOpenGLFunctions_3_3_Core *G;
    GLuint _vao, _vbo, _tex;
    GLint _vertex_position_i, _vertex_normal_i, _vertex_uv_i, _vmp_i, _tex_i;
    QOpenGLShaderProgram _program;
};

