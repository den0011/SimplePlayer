#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>

namespace Ui {
class ControlPanel;
}

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);
    ~ControlPanel();

    // Геттеры для элементов управления
    class QSlider* positionSlider() const;
    class QSlider* volumeSlider() const;
    class QPushButton* btnOpen() const;
    class QPushButton* btnPlayPause() const;
    class QPushButton* btnStop() const;
    class QPushButton* btnPrev() const;
    class QPushButton* btnNext() const;
    class QPushButton* btnDelete() const;
    class QPushButton* btnCopy() const;
    class QPushButton* btnProperties() const;
    class QPushButton* btnClear() const;
    class QPushButton* btnSavePlaylist() const;
    class QPushButton* btnLoadPlaylist() const;
    class QPushButton* btnMute() const;
    class QLabel* lblTime() const;
    class QLabel* lblVolume() const;

    void setTimeText(const QString &text);
    void setVolumeText(int volume);

signals:
    void positionSliderPressed();
    void positionSliderReleased();
    void positionSliderMoved(int position);
    void volumeSliderMoved(int volume);
    void muteToggled(bool muted);

private:
    Ui::ControlPanel *ui;
};

#endif // CONTROLPANEL_H
