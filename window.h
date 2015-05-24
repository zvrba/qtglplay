#pragma once

#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include "ProjectiveWidget.h"

class ProjectiveWidget;

class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = 0);

protected:
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

private:
    ProjectiveWidget *_projectiveWidget;
    QSlider *_uSlider, *_vSlider, *_hSlider;
    QLineEdit *_fov, *_segments;
    QPushButton *_compileButton;
    QTextEdit *_compileLog;

    int _segmentCount;

    QSlider *createSlider(int low, int high);
};

