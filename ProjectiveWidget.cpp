#include <complex>
#include <QKeyEvent>
#include <QVector3D>
#include "ProjectiveWidget.h"
#include "SurfaceGenerator.h"

// TODO! Move geometry generation into the vertex shader!
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
        static const float pi = 3.1416f;
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
    _program(this)
{
}

ProjectiveWidget::~ProjectiveWidget()
{
    cleanup();
}

void ProjectiveWidget::setSegmentCount(int count)
{
    if (count < 8) count = 8;
    _segmentCount = count;
    makeCurrent();
    setupGeometry();
    doneCurrent();
    update();
}

void ProjectiveWidget::setCameraU(int u)
{
    if (u < 0 || u >= _segmentCount) u = 0;
    _cameraU = u;
    setupCamera();
    update();
}

void ProjectiveWidget::setCameraV(int v)
{
    if (v < 0 || v >= _segmentCount) v = 0;
    _cameraV = v;
    setupCamera();
    update();
}

void ProjectiveWidget::setCameraHeight(float height)
{
    if (height < 0) height = 0;
    _cameraHeight = height;
    setupCamera();
    update();
}

void ProjectiveWidget::compileShaders()
{
    makeCurrent();
    _program.release();
    loadProgram();
    doneCurrent();
    update();
}

void ProjectiveWidget::initializeGL()
{
    QOpenGLContext *context = this->context();
    connect(context, &QOpenGLContext::aboutToBeDestroyed, this, &ProjectiveWidget::cleanup);

    G = context->versionFunctions<QOpenGLFunctions_3_3_Core>();
    G->initializeOpenGLFunctions();

    G->glEnable(GL_TEXTURE_2D);
    G->glEnable(GL_BLEND);
    G->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G->glClearColor(0, 0, 0, 1);

    G->glGenVertexArrays(1, &_vao);
    G->glGenBuffers(1, &_vbo);
    G->glGenTextures(1, &_tex);

    _segmentCount = 128;
    _cameraU = _cameraV = 0;
    _cameraHeight = 0;

    loadProgram();
    setupGeometry();
    setupTexture();
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
    doneCurrent();
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
    setupCamera();
}

void ProjectiveWidget::setupCamera()
{

    _xform = _cameraXform;
}

void ProjectiveWidget::setupGeometry()
{
    ProjectiveGenerator bg(256, 256);
    auto shapeData = bg.generate();
    _vertexCount = bg.getVertexCount();
    _triangleCount = bg.getTriangleCount();

    G->glBindVertexArray(_vao);

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
    QImage img("Shaders/TextureBW.png", "PNG");
    if (img.isNull()) {
        qDebug() << "FAILED TO LOAD TEXTURE\n";
        return;
    }

    auto bits = img.constBits();
    auto width = img.width();
    auto height = img.height();

    G->glActiveTexture(GL_TEXTURE0);
    G->glBindTexture(GL_TEXTURE_2D, _tex);
    G->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    G->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    G->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // GRRR, img.format() is RGB32 with alpha forced to 0xFF
    G->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);
}

void ProjectiveWidget::loadProgram()
{
    QString compileMessages;

    _program.removeAllShaders();

    compileMessages = "VERTEX SHADER LOG:\n";
    if (!_program.addShaderFromSourceFile(QOpenGLShader::Vertex, "Shaders/Perspective.txt"))
        compileMessages += _program.log() + "\n";

    compileMessages += "FRAGMENT SHADER LOG:\n";
    if (!_program.addShaderFromSourceFile(QOpenGLShader::Fragment, "Shaders/Fragment.txt"))
        compileMessages += _program.log() + "\n";

    compileMessages += "LINK LOG:\n";
    if (!_program.link())
        compileMessages += _program.log() + "\n";
    
    _program.bind();

    GLuint p = _program.programId();

    // WTF? glGetAttribLocation will return -1 for vertex_normal unless it is somehow used in the program.
    // Hardcode this for now.
#if 0
    _vertex_position_i = G->glGetAttribLocation(p, "vertex_position");
    _vertex_normal_i = G->glGetAttribLocation(p, "vertex_normal");
    _vertex_uv_i = G->glGetAttribLocation(p, "vertex_uv");
#else
    if (G->glGetAttribLocation(p, "vertex_normal") < 0)
      compileMessages += "VERTEX NORMAL OPTIMIZED OUT\n";

    _vertex_position_i = 0;
    _vertex_normal_i = 1;
    _vertex_uv_i = 2;
#endif

    _vmp_i = G->glGetUniformLocation(p, "vmp");
    _tex_i = G->glGetUniformLocation(p, "tex");

    emit compilationDone(compileMessages);
}
