#include "controlpanel.h"
#include "ui_controlpanel.h"

ControlPanel::ControlPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanel)
{
    ui->setupUi(this);

    // Настраиваем соединения для сигналов
    connect(ui->positionSlider, &QSlider::sliderPressed,
            this, &ControlPanel::positionSliderPressed);
    connect(ui->positionSlider, &QSlider::sliderReleased,
            this, &ControlPanel::positionSliderReleased);
    connect(ui->positionSlider, &QSlider::sliderMoved,
            this, [this](int position) { emit positionSliderMoved(position); });
    connect(ui->volumeSlider, &QSlider::valueChanged,
            this, [this](int volume) { emit volumeSliderMoved(volume); });
    connect(ui->btnMute, &QPushButton::toggled,
            this, &ControlPanel::muteToggled);
}

ControlPanel::~ControlPanel()
{
    delete ui;
}

QSlider* ControlPanel::positionSlider() const { return ui->positionSlider; }
QSlider* ControlPanel::volumeSlider() const { return ui->volumeSlider; }
QPushButton* ControlPanel::btnOpen() const { return ui->btnOpen; }
QPushButton* ControlPanel::btnPlayPause() const { return ui->btnPlayPause; }
QPushButton* ControlPanel::btnStop() const { return ui->btnStop; }
QPushButton* ControlPanel::btnPrev() const { return ui->btnPrev; }
QPushButton* ControlPanel::btnNext() const { return ui->btnNext; }
QPushButton* ControlPanel::btnDelete() const { return ui->btnDelete; }
QPushButton* ControlPanel::btnCopy() const { return ui->btnCopy; }
QPushButton* ControlPanel::btnProperties() const { return ui->btnProperties; }
QPushButton* ControlPanel::btnClear() const { return ui->btnClear; }
QPushButton* ControlPanel::btnSavePlaylist() const { return ui->btnSavePlaylist; }
QPushButton* ControlPanel::btnLoadPlaylist() const { return ui->btnLoadPlaylist; }
QPushButton* ControlPanel::btnMute() const { return ui->btnMute; }
QLabel* ControlPanel::lblTime() const { return ui->lblTime; }
QLabel* ControlPanel::lblVolume() const { return ui->lblVolume; }

void ControlPanel::setTimeText(const QString &text)
{
    ui->lblTime->setText(text);
}

void ControlPanel::setVolumeText(int volume)
{
    if (volume == 0) {
        ui->lblVolume->setText("🔇");
    } else if (volume < 33) {
        ui->lblVolume->setText("🔈");
    } else if (volume < 66) {
        ui->lblVolume->setText("🔉");
    } else {
        ui->lblVolume->setText("🔊");
    }
}
