#include <complex>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

ProjectiveWidget::ProjectiveWidget(QWidget*) : _vbo(QOpenGLBuffer::VertexBuffer)
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

    _rx = _ry = _rz = _tz = 0;
    _znear = 1;
    // resizeGL called also after initialization
}

void ProjectiveWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QOpenGLVertexArrayObject::Binder binder(&_vao);
    //glDrawArrays(GL_TRIANGLES, 0, _triangleCount*3);

    for (int i = 0; i < _triangleCount; ++i)
        glDrawArrays(GL_LINE_LOOP, 3*i, 3);

    glFlush();
}

void ProjectiveWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    setupXform();
}

void ProjectiveWidget::setupXform()
{
    {
        glm::mat4 I(1); // start with identity matrix
        auto rx = rotate(I, radians(_rx), glm::vec3(1, 0, 0));
        auto ry = rotate(rx, radians(_ry), glm::vec3(0, 1, 0));
        auto rz = rotate(ry, radians(_rz), glm::vec3(0, 0, 1));
        _objectXform = rz;
    }

    {
        _perspXform = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, _znear / 10.0f, 100.0f);
    }

    _xform = _perspXform * _objectXform;
    glUniformMatrix4fv(_vmpLocation, 1, GL_FALSE, glm::value_ptr(_xform));
}

void ProjectiveWidget::keyPressEvent(QKeyEvent *ev)
{
    return; // NOP
}

void ProjectiveWidget::cleanup()
{
    makeCurrent();
    _vao.destroy();
    _vbo.destroy();
    _program.release();
}

void ProjectiveWidget::setupGeometry()
{
    BoysGenerator bg(128, 128);
    auto shapeData = bg.generate();
    _vertexCount = bg.getVertexCount();
    _triangleCount = bg.getTriangleCount();

    _vao.create();
    QOpenGLVertexArrayObject::Binder binder(&_vao);

    _vbo.create();
    _vbo.bind();
    _vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    _vbo.allocate(&shapeData[0], shapeData.size() * sizeof(float));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
}

void ProjectiveWidget::setupProgram()
{
    if (!_program.addShaderFromSourceFile(QOpenGLShader::Vertex, "Shaders/Perspective.txt"))
        qDebug() << "VERTEX SHADER LOG: " << _program.log();

    if (!_program.addShaderFromSourceFile(QOpenGLShader::Fragment, "Shaders/FragmentTest.txt"))
        qDebug() << "FRAGMENT SHADER LOG: " << _program.log();

    _program.link();
    _program.bind();
    _vmpLocation = _program.uniformLocation("ViewModelProject");
    if (_vmpLocation < 0)
        qDebug() << "uniform location not found";
}

