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
    QOpenGLContext *context = this->context();
    connect(context, &QOpenGLContext::aboutToBeDestroyed, this, &ProjectiveWidget::cleanup);

    G = context->versionFunctions<QOpenGLFunctions_3_3_Core>();
    G->initializeOpenGLFunctions();
    G->glEnable(GL_TEXTURE_2D);
    G->glClearColor(0, 0, 0, 1);

    loadProgram();
    setupGeometry();
    setupTexture();
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
    G->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    G->glUniformMatrix4fv(_vmp_i, 1, GL_FALSE, _xform.data());
    G->glBindVertexArray(_vao);
    G->glBindTexture(GL_TEXTURE_2D, _tex);
    G->glUniform1i(_tex_i, 0);

#if 1
    G->glDrawArrays(GL_TRIANGLES, 0, _triangleCount*3);
#else
    for (int i = 0; i < _triangleCount; ++i)
        G->glDrawArrays(GL_LINE_LOOP, 3*i, 3);
#endif

    G->glFlush();
    G->glBindVertexArray(0);
}

void ProjectiveWidget::resizeGL(int width, int height)
{
    G->glViewport(0, 0, width, height);
    setupXform();
}

void ProjectiveWidget::setupXform()
{
    {
        _objectXform.setToIdentity();
        _objectXform.translate(0, 0, _tz / 10.0f);
        _objectXform.rotate(_rx, 1, 0, 0);
        _objectXform.rotate(_ry, 0, 1, 0);
        _objectXform.rotate(_rz, 0, 0, 1);
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
    G->glBindVertexArray(0);
    G->glDeleteVertexArrays(1, &_vao);
    G->glDeleteBuffers(1, &_vbo);
    G->glBindTexture(GL_TEXTURE_2D, 0);
    G->glDeleteTextures(1, &_tex);
    _program.release();
}

void ProjectiveWidget::setupGeometry()
{
    BoysGenerator bg(32, 32);
    auto shapeData = bg.generate();
    _vertexCount = bg.getVertexCount();
    _triangleCount = bg.getTriangleCount();

    G->glGenVertexArrays(1, &_vao);
    G->glBindVertexArray(_vao);

    G->glGenBuffers(1, &_vbo);
    G->glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    G->glBufferData(GL_ARRAY_BUFFER, shapeData.size() * sizeof(float), &shapeData[0], GL_STATIC_DRAW);

    G->glVertexAttribPointer(_vertex_position_i, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    G->glEnableVertexAttribArray(_vertex_position_i);

    G->glVertexAttribPointer(_vertex_uv_i, 2, GL_FLOAT, GL_FALSE, 0, (void*)(_triangleCount*2*3*3*sizeof(float)));
    G->glEnableVertexAttribArray(_vertex_uv_i);

    G->glBindVertexArray(0);
}

void ProjectiveWidget::setupTexture()
{
    QImage img("Shaders/Texture.png", "PNG");
    if (img.isNull()) {
        qDebug() << "FAILED TO LOAD TEXTURE\n";
        return;
    }

    auto bits = img.constBits();
    auto bpp = img.bitPlaneCount();

    G->glActiveTexture(GL_TEXTURE0);
    G->glGenTextures(1, &_tex);
    G->glBindTexture(GL_TEXTURE_2D, _tex);
    G->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.width(), img.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bits);
    G->glBindTexture(GL_TEXTURE_2D, 0);
}

void ProjectiveWidget::loadProgram()
{
    if (!_program.addShaderFromSourceFile(QOpenGLShader::Vertex, "Shaders/Perspective.txt"))
        qDebug() << "VERTEX SHADER LOG: " << _program.log();

    if (!_program.addShaderFromSourceFile(QOpenGLShader::Fragment, "Shaders/Fragment.txt"))
        qDebug() << "FRAGMENT SHADER LOG: " << _program.log();

    _program.link();
    _program.bind();

    _vertex_position_i = _program.attributeLocation("vertex_position");
    _vertex_uv_i = _program.attributeLocation("vertex_uv");
    _vmp_i = _program.uniformLocation("ViewModelProject");
    _tex_i = _program.uniformLocation("tex");
}
