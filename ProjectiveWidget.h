// The very first example from the red book, adapted to QT.

#pragma once
#ifndef PROJECTIVE_WIDGET_H
#define PROJECTIVE_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

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

private:
    void setupGeometry();
    void setupProgram();
    void cleanup();


    QOpenGLVertexArrayObject _vertexArray;
    QOpenGLBuffer _vertexBuffer;
    QOpenGLShaderProgram _shaderProgram;
};

#endif