#include "ProjectiveWidget.h"

ProjectiveWidget::ProjectiveWidget(QWidget*) : _vertexBuffer(QOpenGLBuffer::VertexBuffer)
{
}

ProjectiveWidget::~ProjectiveWidget()
{
    cleanup();
}

void ProjectiveWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ProjectiveWidget::cleanup);
    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 1);

    setupGeometry();
    setupProgram();
}

void ProjectiveWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QOpenGLVertexArrayObject::Binder binder(&_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glFlush();
}

void ProjectiveWidget::resizeGL(int, int)
{
    return; // NOP
}

void ProjectiveWidget::cleanup()
{
    makeCurrent();
    _vertexArray.destroy();
    _vertexBuffer.destroy();
    _shaderProgram.release();
}

void ProjectiveWidget::setupGeometry()
{
    static const GLfloat vertexData[] =
    {
        -0.90, -0.90,
         0.85, -0.90,
        -0.90,  0.85,
         0.90, -0.85,
         0.90,  0.90,
        -0.85,  0.90
    };

    _vertexArray.create();
    QOpenGLVertexArrayObject::Binder binder(&_vertexArray);

    _vertexBuffer.create();
    _vertexBuffer.bind();
    _vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    _vertexBuffer.allocate(vertexData, sizeof(vertexData));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
}

void ProjectiveWidget::setupProgram()
{
    if (!_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, "Shaders/VertexTest.txt"))
        qDebug() << "VERTEX SHADER LOG: " << _shaderProgram.log();

    if (!_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, "Shaders/FragmentTest.txt"))
        qDebug() << "FRAGMENT SHADER LOG: " << _shaderProgram.log();

    _shaderProgram.link();
    _shaderProgram.bind();
}
