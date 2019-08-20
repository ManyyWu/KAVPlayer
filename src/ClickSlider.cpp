#include <QCoreApplication>
#include <QMouseEvent>
#include "ClickSlider.h"
#include "moc_ClickSlider.cpp"

void ClickSlider::mousePressEvent (QMouseEvent * e)
{
    QSlider::mousePressEvent(e);

    double pos = e->pos().x() / (double)width();
    setValue(pos * (maximum() - minimum()) + minimum());
    emit clicked();
}

ClickSlider::ClickSlider (QWidget * parent)
    : QSlider (parent)
{
}

ClickSlider::ClickSlider (Qt::Orientation orientation, QWidget * parent)
    :QSlider (orientation, parent)
{
}


