#include <QKeyEvent>
#include <QVector3D>
#include <QtMath>
#include "ProjectiveWidget.h"
#include "SurfaceGenerator.h"

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

void ProjectiveWidget::setCameraHeight(int height)
{
    if (height < 0) height = 0;
    _cameraHeight = height / 16.0f;
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
    G->glGenBuffers(3, _vbo);
    G->glGenTextures(1, &_tex);

    _segmentCount = 128;
    _cameraU = _cameraV = _cameraHeading = _cameraHeight = _cameraTilt = 0;
    _cameraFOV = 15;

    loadProgram();
    setupGeometry();
    setupTexture();
}

void ProjectiveWidget::cleanup()
{
    makeCurrent();
    G->glDeleteVertexArrays(1, &_vao);
    G->glDeleteBuffers(3, _vbo);
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

    G->glBindVertexArray(0);
    G->glFlush();
}

void ProjectiveWidget::resizeGL(int width, int height)
{
    G->glViewport(0, 0, width, height);
    _vpWidth = width; _vpHeight = height;
    setupCamera();
}

void ProjectiveWidget::keyPressEvent(QKeyEvent *ev)
{
    const float uvstep = 1e-2f;
    const float htstep = 1e-1f;
    bool changed1 = true, changed2 = true;

    {
        float move = 0;

        switch (ev->key())
        {
        case Qt::Key_Down: move = -uvstep; break;
        case Qt::Key_Up: move = uvstep; break;
        case Qt::Key_Left: _cameraHeading = fmodf(_cameraHeading + 360 - 15, 360); break;
        case Qt::Key_Right: _cameraHeading = fmodf(_cameraHeading + 15, 360); break;
        default: changed1 = false;
        }

        _cameraU = fmodf(_cameraU + 1 + uvstep*cos(qDegreesToRadians(_cameraHeading)), 1);
        _cameraV = fmodf(_cameraV + 1 + uvstep*sin(qDegreesToRadians(_cameraHeading)), 1);

        if (changed1)
            emit cameraSurfacePositionChanged(QVector3D(_cameraU, _cameraV, _cameraHeading));
    }

    {
        if (ev->text().isEmpty())
            return;

        char key = ev->text().toLatin1().at(0);

        switch (key)
        {
        case 'H': _cameraHeight += htstep; break;
        case 'h': _cameraHeight -= htstep; break;
        case 'T': _cameraTilt += htstep; break;
        case 't': _cameraTilt -= htstep; break;
        case 'F': _cameraFOV = qMax(60.f, _cameraFOV+5); break;
        case 'f': _cameraFOV = qMin(5.f, _cameraFOV-5); break;
        default: changed2 = false;
        }

        if (changed2)
            emit cameraProjectionTargetChanged(QVector3D(_cameraHeight, _cameraTilt, _cameraFOV));
    }

    if (changed1 || changed2)
        update();
}

void ProjectiveWidget::setupCamera()
{
    QMatrix4x4 cameraXform, perspXform;

    {
        const int i = 6 * _shapeData.uvIndex(_cameraU, _cameraV);

        auto triangles = _shapeData.getTriangles();
        QVector3D eye = triangles[i];    // 6 value per uv index
        QVector3D center = triangles[i+1];
        center.setZ(_cameraHeight-1);

        auto normals = _shapeData.getNormals();
        QVector3D up(0, normals[i].y(), 0);// = normals[i];

        cameraXform.lookAt(eye, center, up);
    }

    perspXform.perspective(_cameraFOV, (float)_vpWidth / _vpHeight, 1e-3f, 1e4f);

    _xform = perspXform * cameraXform;
}

void ProjectiveWidget::setupGeometry()
{
    _shapeData.generate(_segmentCount, _segmentCount, true, true);

    G->glBindVertexArray(_vao);

    auto triangles = _shapeData.getTriangles();
    G->glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
    G->glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(QVector3D), &triangles[0], GL_STATIC_DRAW);
    G->glVertexAttribPointer(_vertex_position_i, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    G->glEnableVertexAttribArray(_vertex_position_i);
    _triangleCount = triangles.size() / 3;

    auto normals = _shapeData.getNormals();
    G->glBindBuffer(GL_ARRAY_BUFFER, _vbo[1]);
    G->glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(QVector3D), &normals[0], GL_STATIC_DRAW);
    G->glVertexAttribPointer(_vertex_normal_i, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    G->glEnableVertexAttribArray(_vertex_normal_i);

    auto uvs = _shapeData.getUVs();
    G->glBindBuffer(GL_ARRAY_BUFFER, _vbo[2]);
    G->glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(QVector2D), &uvs[0], GL_STATIC_DRAW);
    G->glVertexAttribPointer(_vertex_uv_i, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    G->glEnableVertexAttribArray(_vertex_uv_i);

    G->glBindVertexArray(0);
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
