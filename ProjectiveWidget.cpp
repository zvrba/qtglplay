#include <complex>
#include "ProjectiveWidget.h"
#include "SurfaceGenerator.h"

class BoysGenerator : public SurfaceGenerator
{
    using complex = std::complex<double>;

    static complex fromPolar(double r, double phi)
    {
        return complex(r*cos(phi), r*sin(phi));
    }

    static glm::vec3 bryant(complex z);

protected:
    virtual glm::vec2 UV(int u, int v) const override
    {
        return glm::vec2((float)u / getUSegmentCount(), (float)v / getVSegmentCount()); // TODO: should not be linear.
    }

    virtual glm::vec3 F(glm::vec2 uv) const override
    {
        return bryant(fromPolar(uv.x, uv.y));
    }

public:
    BoysGenerator(int uSegments, int vSegments) : SurfaceGenerator(uSegments, vSegments, true, true)
    { }
};

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

    return glm::vec3(g1 / g, g2 / g, g3 / g);
}

/////////////////////////////////////////////////////////////////////////////

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

