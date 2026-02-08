#include "clickableslider.h"
#include <QApplication>

ClickableSlider::ClickableSlider(QWidget *parent)
    : QSlider(parent)
    , m_isDragging(false)
    , m_clickWithoutDrag(false)
{
    setTracking(false); // Отключаем tracking для лучшего контроля
}

ClickableSlider::ClickableSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
    , m_isDragging(false)
    , m_clickWithoutDrag(false)
{
    setTracking(false);
}

void ClickableSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Получаем геометрию ползунка
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

        // Если клик не на ползунке, устанавливаем значение сразу
        if (!sliderRect.contains(event->pos())) {
            // Вычисляем новое значение на основе позиции клика
            int newValue = QStyle::sliderValueFromPosition(
                minimum(),
                maximum(),
                event->pos().x(),
                width());

            // Устанавливаем новое значение
            setValue(newValue);

            // Сигнализируем об изменении
            emit sliderPressed();
            emit sliderMoved(newValue);
            emit sliderClicked(newValue);  // Новый сигнал

            m_isDragging = true;
            m_clickWithoutDrag = true;
            event->accept();
            return;
        } else {
            // Клик на ползунке - обычное поведение
            m_clickWithoutDrag = false;
        }
    }

    // Для обычного перетаскивания ползунка
    QSlider::mousePressEvent(event);
    m_isDragging = true;
}

void ClickableSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        if (m_clickWithoutDrag) {
            // Если это был клик без перетаскивания, все равно обновляем значение
            int newValue = QStyle::sliderValueFromPosition(
                minimum(),
                maximum(),
                event->pos().x(),
                width());

            setValue(newValue);
            emit sliderMoved(newValue);
        } else {
            // Обычное перетаскивание
            QSlider::mouseMoveEvent(event);
        }

        event->accept();
    } else {
        QSlider::mouseMoveEvent(event);
    }
}

void ClickableSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isDragging) {
        if (m_clickWithoutDrag) {
            // Если это был просто клик (без перетаскивания ползунка)
            emit sliderReleased();
        }
        m_isDragging = false;
        m_clickWithoutDrag = false;
        event->accept();
    } else {
        QSlider::mouseReleaseEvent(event);
    }
}
