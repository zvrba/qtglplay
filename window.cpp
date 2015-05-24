#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QDesktopWidget>
#include <QApplication>
#include <QFormLayout>
#include <QValidator>
#include "window.h"

Window::Window(QWidget *parent) : QWidget(parent)
{
    _segmentCount = 128;

    _projectiveWidget = new ProjectiveWidget;

    _uSlider = createSlider(0, _segmentCount);
    connect(_uSlider, SIGNAL(valueChanged(int)), _projectiveWidget, SLOT(setCameraU(int)));

    _vSlider = createSlider(0, _segmentCount);
    connect(_vSlider, SIGNAL(valueChanged(int)), _projectiveWidget, SLOT(setCameraV(int)));

    _hSlider = createSlider(0, 32);
    connect(_hSlider, SIGNAL(valueChanged(int)), _projectiveWidget, SLOT(setCameraHeight(int)));

    _fov = new QLineEdit("1");
    _fov->setValidator(new QDoubleValidator(0.1, 10, 2));

    _segments = new QLineEdit("128");
    _segments->setValidator(new QIntValidator(16, 512));

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("U position", _uSlider);
    formLayout->addRow("V position", _vSlider);
    formLayout->addRow("H position", _hSlider);
    formLayout->addRow("FOV", _fov);
    formLayout->addRow("Segments", _segments);


    _compileButton = new QPushButton("Compile shaders");
    _compileLog = new QTextEdit("(compile log)");
    _compileLog->setReadOnly(true);

    QVBoxLayout *settingsLayout = new QVBoxLayout;
    settingsLayout->addLayout(formLayout);
    settingsLayout->addWidget(_compileButton);
    settingsLayout->addWidget(_compileLog);


    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(_projectiveWidget);
    mainLayout->addLayout(settingsLayout);

    setLayout(mainLayout);

    setWindowTitle("Projective");
}

QSlider *Window::createSlider(int low, int high)
{
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(low, high);
    slider->setSingleStep(1);
    slider->setPageStep(8);
    slider->setTickInterval(16);
    slider->setTickPosition(QSlider::TicksBelow);
    return slider;
}

void Window::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}
