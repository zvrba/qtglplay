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

/////////////////////////////////////////////////////////////////////////////

void BoysGenerator::bryantsParametrization(int uSegments, int vSegments)
{
    glm::vec3 quadVertex[4];
    glm::vec2 quadUV[4];

    _uvs.reserve(uSegments*vSegments);
    _vertices.reserve(uSegments*vSegments);
    _normals.reserve(uSegments*vSegments);
    _normalCounts.reserve(uSegments*vSegments);

    // Generate UVs and vertex coordinates.

    for (int u = 0; u < uSegments; ++u)
    for (int v = 0; v < vSegments; ++v)
    {
        const double r = (double)u / uSegments;
        const double phi = (double)v / vSegments;

        _uvs.push_back(glm::vec2(r, phi));
        _vertices.push_back(glm::vec3(bryant(fromPolar(r, phi))));
    }

    generateTriangles(uSegments, vSegments);
    generateNormals(uSegments, vSegments);
}

void BoysGenerator::generateTriangles(int uSegments, int vSegments)
{
    for (int u = 0; u < uSegments; ++u)
    for (int v = 0; v < vSegments; ++v)
    {
        generateTriangle(u, (u+1) % uSegments, 
    }
}

void BoysGenerator::generateTriangle(int i0, int i1, int i2)
{
    auto v0 = _vertices[i0], v1 = _vertices[i1], v2 = _vertices[i2];

}

glm::vec3 BoysGenerator::bryant(complex z)
{
    // TODO! FIX! This will be much more precise in polar coordinates!
    complex num1 = z * (complex(1) - pow(z, 4));
    complex num2 = z * (complex(1) + pow(z, 4));
    complex num3 = complex(1) + pow(z, 6);
    complex den = pow(z, 6) + complex(sqrt(5)) * pow(z, 3) - complex(1);

    double g1 = -1.5 * (num1 / den).imag();
    double g2 = -1.5 * (num2 / den).real();
    double g3 = (num3 / den).imag() - 0.5;
    double g = g1*g1 + g2*g2 + g3*g3;

    return glm::vec3(g1/g, g2/g, g3/g);
}

BoysGenerator::complex BoysGenerator::fromPolar(double r, double phi)
{
    return complex(r*cos(phi), r*sin(phi));
}

