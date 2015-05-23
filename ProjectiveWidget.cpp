#include <complex>
#include <QKeyEvent>
#include <QVector3D>
#include "ProjectiveWidget.h"
#include "SurfaceGenerator.h"

class QuadGenerator : public SurfaceGenerator
{
protected:
    virtual QVector2D UV(int u, int v) const override
    {
        return QVector2D((float)u / (getUSegmentCount()-1), (float)v / (getVSegmentCount()-1));
    }
    virtual QVector3D F(QVector2D uv) const override
    {
        return QVector3D(uv.x(), uv.y(), uv.x()+uv.y());
    }
public:
    QuadGenerator(int uSegments, int vSegments) : SurfaceGenerator(uSegments, vSegments, false, false)
    { }
};

class ProjectiveGenerator : public SurfaceGenerator
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
        float uu = (float)u / (getUSegmentCount()-1);
        float vv = (float)v / (getVSegmentCount()-1);
        return QVector2D(uu, vv);
    }

    virtual QVector3D F(QVector2D uv) const override
    {
        static const float pi = 3.1416;
        double u = uv.x() * 2*pi, v = uv.y() * pi/2;
        double cosu = cos(u), cosv = cos(v), sinu = sin(u), sinv = sin(v), sin2v = sin(2*v);
        double x = cosu * sin2v;
        double y = sinu * sin2v;
        double z = cosv*cosv - cosu*cosu * sinv*sinv;
        return QVector3D(x, y, z);
    }

public:
    ProjectiveGenerator(int uSegments, int vSegments) : SurfaceGenerator(uSegments, vSegments, true, true)
    { }
};

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
    //G->glEnable(GL_BLEND);
    G->glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
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
    _tz = -30;
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
        //_perspXform.ortho(-0.2, 1.2, -0.2, 1.2, 0.1, 5);
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
        qDebug() << "T: " << _tz << " N: " << _znear;
        return;
    case '!':
        qDebug() << "COMPILING";
        makeCurrent();
        _program.release();
        loadProgram();
        doneCurrent();
        qDebug() << "COMPILE FINISHED";
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
    }

    update();
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
    ProjectiveGenerator bg(128, 128);
    //QuadGenerator bg(2, 2);
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

    G->glVertexAttribPointer(_vertex_normal_i, 3, GL_FLOAT, GL_FALSE, 0, (void*)(_triangleCount*3*3*sizeof(float)));
    G->glEnableVertexAttribArray(_vertex_normal_i);

    G->glVertexAttribPointer(_vertex_uv_i, 2, GL_FLOAT, GL_FALSE, 0, (void*)(_triangleCount*2*3*3*sizeof(float)));
    G->glEnableVertexAttribArray(_vertex_uv_i);
}

void ProjectiveWidget::setupTexture()
{
    QImage img("Shaders/Texture.png", "PNG");
    if (img.isNull()) {
        qDebug() << "FAILED TO LOAD TEXTURE\n";
        return;
    }

    auto bits = img.constBits();
    auto width = img.width();
    auto height = img.height();
    auto bpp = img.bitPlaneCount();
    auto format = img.format();


    G->glActiveTexture(GL_TEXTURE0);
    G->glGenTextures(1, &_tex);
    G->glBindTexture(GL_TEXTURE_2D, _tex);
    G->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    G->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    G->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // GRRR, img.format() is RGB32 with alpha forced to 0xFF
    G->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);
}

void ProjectiveWidget::loadProgram()
{
    _program.removeAllShaders();

    if (!_program.addShaderFromSourceFile(QOpenGLShader::Vertex, "Shaders/Perspective.txt"))
        qDebug() << "VERTEX SHADER LOG: " << _program.log();

    if (!_program.addShaderFromSourceFile(QOpenGLShader::Fragment, "Shaders/Fragment.txt"))
        qDebug() << "FRAGMENT SHADER LOG: " << _program.log();

    _program.link();
    _program.bind();

    _vertex_position_i = _program.attributeLocation("vertex_position");
    _vertex_normal_i = _program.attributeLocation("vertex_normal");
    _vertex_uv_i = _program.attributeLocation("vertex_uv");
    _vmp_i = _program.uniformLocation("vmp");
    _tex_i = _program.uniformLocation("tex");
}
