#ifndef _QCLICKSLIDER_H_
#define _QCLICKSLIDER_H_

#include <Qt>
#include <QSlider>
#include <QMouseEvent>

class ClickSlider : public QSlider
{
    Q_OBJECT

Q_SIGNALS:
    void     clicked         ();   

protected:
    void     mousePressEvent (QMouseEvent *e);

public:
    explicit ClickSlider     (QWidget *parent = nullptr);
    explicit ClickSlider     (Qt::Orientation orientation, QWidget *parent = nullptr);
};

#endif /* _QCLICKSLIDER_H_ */
