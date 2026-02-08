#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

#include <QSlider>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

class ClickableSlider : public QSlider
{
    Q_OBJECT

public:
    explicit ClickableSlider(QWidget *parent = nullptr);
    explicit ClickableSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

signals:
    void sliderClicked(int value);  // Новый сигнал для клика

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_isDragging;
    bool m_clickWithoutDrag;
};

#endif // CLICKABLESLIDER_H
