#include <complex>
#include <QKeyEvent>
#include <QVector3D>
#include "ProjectiveWidget.h"
#include "SurfaceGenerator.h"

class BoysGenerator : public SurfaceGenerator
{
    using complex = std::complex<double>;

    static complex fromPolar(double r, double phi)
    {
        return complex(r*cos(phi), r*sin(phi));
    }

    static QVector3D bryant(complex z);

protected:
    virtual QVector2D UV(int u, int v) const override
    {
        return QVector2D((float)u / getUSegmentCount(), (float)v / getVSegmentCount()); // TODO: should not be linear.
    }

    virtual QVector3D F(QVector2D uv) const override
    {
        return bryant(fromPolar(uv.x(), uv.y() * 2 * 3.141593f));
    }

public:
    BoysGenerator(int uSegments, int vSegments) : SurfaceGenerator(uSegments, vSegments, true, true)
    { }
};

QVector3D BoysGenerator::bryant(complex z)
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

    return QVector3D(g1 / g, g2 / g, g3 / g);
}

/////////////////////////////////////////////////////////////////////////////

ProjectiveWidget::ProjectiveWidget(QWidget*) : 
    _vbo(QOpenGLBuffer::VertexBuffer),
    _program(this)
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

    loadProgram();
    setupGeometry();
    resetXform();
    // resizeGL called also after initialization
}

void ProjectiveWidget::resetXform()
{
    _rx = _ry = _rz = 0;
    _tz = 0;
    _znear = 1;
}

void ProjectiveWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _program.setUniformValue("ViewModelProject", _xform);
    QOpenGLVertexArrayObject::Binder binder(&_vao);
    //GLenum err = glGetError();
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
        _objectXform.setToIdentity();
        _objectXform.rotate(_rx, 1, 0, 0);
        _objectXform.rotate(_ry, 0, 1, 0);
        _objectXform.rotate(_rz, 0, 0, 1);
        _objectXform.translate(0, 0, _tz / 10.0f);
    }

    {
        _perspXform.setToIdentity();
        _perspXform.frustum(-1.0f, 1.0f, -1.0f, 1.0f, _znear / 10.0f, 100.0f);
    }

    _xform = _perspXform * _objectXform;
}

// lower-case: lower; upper-case: higher value
// xX,yY,zZ: rotations around these axes
// tT: translation along Z axis
// nN: near plane
// space: resets all transforms to defaults
void ProjectiveWidget::keyPressEvent(QKeyEvent *ev)
{
    int *target = nullptr;
    int delta = 0;

    if (ev->text().isEmpty())   // Happens for modifiers (shift, etc)
        return;

    switch (ev->text().toUpper().at(0).toLatin1())
    {
    case 'X': target = &_rx; delta = 12; break;
    case 'Y': target = &_ry; delta = 12; break;
    case 'Z': target = &_rz; delta = 12; break;
    case 'T': target = &_tz; delta = 1; break;
    case 'N': target = &_znear; delta = 1; break;
    case ' ':
        qDebug() << "R: " << _rx << " " << _ry << " " << _rz;
        qDebug() << "T: " << _tz;
        break;
    default:
        qDebug() << "UNHANDLED KEY: " << ev->text();
        return;
    }
    
    if (target)
    {
        if (ev->text() != ev->text().toUpper())
            delta = -delta;
        *target += delta;
        setupXform();
        update();
    }
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
    BoysGenerator bg(32, 32);
    auto shapeData = bg.generate();
    _vertexCount = bg.getVertexCount();
    _triangleCount = bg.getTriangleCount();

    _vao.create();
    QOpenGLVertexArrayObject::Binder binder(&_vao);

    _vbo.create();
    _vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    _vbo.bind();
    _vbo.allocate(&shapeData[0], shapeData.size() * sizeof(float));

    _program.setAttributeBuffer("Vertex", GL_FLOAT, 0, 3);
    _program.enableAttributeArray("Vertex");
}

void ProjectiveWidget::loadProgram()
{
    if (!_program.addShaderFromSourceFile(QOpenGLShader::Vertex, "Shaders/Perspective.txt"))
        qDebug() << "VERTEX SHADER LOG: " << _program.log();

    if (!_program.addShaderFromSourceFile(QOpenGLShader::Fragment, "Shaders/FragmentTest.txt"))
        qDebug() << "FRAGMENT SHADER LOG: " << _program.log();

    _program.link();
    _program.bind();
}
