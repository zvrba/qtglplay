#include "RedBookWidget.h"

RedBookWidget::RedBookWidget(QWidget*) : _vertexBuffer(QOpenGLBuffer::VertexBuffer)
{
}

RedBookWidget::~RedBookWidget()
{
    cleanup();
}

void RedBookWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &RedBookWidget::cleanup);
    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 1);

    setupGeometry();
    setupProgram();
}

void RedBookWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QOpenGLVertexArrayObject::Binder binder(&_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glFlush();
}

void RedBookWidget::resizeGL(int, int)
{
    return; // NOP
}

void RedBookWidget::cleanup()
{
    makeCurrent();
    _vertexArray.destroy();
    _vertexBuffer.destroy();
    _shaderProgram.release();
}

void RedBookWidget::setupGeometry()
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

void RedBookWidget::setupProgram()
{
    _shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, "src/VertexTest.txt");
    _shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, "src/FragmentTest.txt");
    _shaderProgram.link();
    _shaderProgram.bind();
}
