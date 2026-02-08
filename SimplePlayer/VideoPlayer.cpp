#include "videoplayer.h"
#include "ui_videoplayer.h"
#include <QTime>
#include <QStandardPaths>

VideoPlayer::VideoPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);

    // Создаем медиаплеер и видеовиджет
    player = new QMediaPlayer(this);
    videoWidget = new QVideoWidget(this);

    player->setVideoOutput(videoWidget);

    // Настраиваем интерфейс
    setupUI();
    setupConnections();

    // Настраиваем окно для drag and drop
    setAcceptDrops(true);

    // Инициализация
    updatePlaylistButtons();
}

VideoPlayer::~VideoPlayer()
{
    delete ui;
}

void VideoPlayer::setupUI()
{
    // Устанавливаем видеовиджет
    ui->verticalLayout->insertWidget(0, videoWidget);

    // Настраиваем слайдеры
    ui->positionSlider->setRange(0, 0);
    ui->volumeSlider->setRange(0, 100);
    ui->volumeSlider->setValue(50);

    // Устанавливаем иконки
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->openButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));

    // Разрешаем перетаскивание в плейлист
    ui->playlistWidget->setDragDropMode(QAbstractItemView::DropOnly);
    ui->playlistWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void VideoPlayer::setupConnections()
{
    // Кнопки управления
    connect(ui->openButton, &QPushButton::clicked, this, &VideoPlayer::onOpenFile);
    connect(ui->playButton, &QPushButton::clicked, this, &VideoPlayer::onPlayPause);
    connect(ui->stopButton, &QPushButton::clicked, this, &VideoPlayer::onStop);

    // Слайдеры
    connect(ui->positionSlider, &QSlider::sliderMoved, this, &VideoPlayer::onSliderMoved);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &VideoPlayer::onVolumeChanged);

    // Медиаплеер
    connect(player, &QMediaPlayer::positionChanged, this, &VideoPlayer::onPositionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &VideoPlayer::onDurationChanged);
    connect(player, &QMediaPlayer::stateChanged, [this](QMediaPlayer::State state) {
        ui->playButton->setIcon(style()->standardIcon(
            state == QMediaPlayer::PlayingState ?
            QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
    });

    // Плейлист
    connect(ui->playlistWidget, &QListWidget::itemDoubleClicked,
            this, &VideoPlayer::onPlaylistItemDoubleClicked);

    // Кнопки плейлиста
    connect(ui->deleteFileButton, &QPushButton::clicked, this, &VideoPlayer::onDeleteFile);
    connect(ui->copyFileButton, &QPushButton::clicked, this, &VideoPlayer::onCopyFile);
    connect(ui->clearPlaylistButton, &QPushButton::clicked, this, &VideoPlayer::onClearPlaylist);
    connect(ui->savePlaylistButton, &QPushButton::clicked, this, &VideoPlayer::onSavePlaylist);
    connect(ui->loadPlaylistButton, &QPushButton::clicked, this, &VideoPlayer::onLoadPlaylist);

    // Таймер для обновления времени
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VideoPlayer::updateTimeDisplay);
    timer->start(1000);
}

void VideoPlayer::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VideoPlayer::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        for (const QUrl &url : urlList) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                addToPlaylist(filePath);
            }
        }

        // Если ничего не воспроизводится, воспроизвести первый файл
        if (player->state() == QMediaPlayer::StoppedState &&
            ui->playlistWidget->count() > 0) {
            ui->playlistWidget->setCurrentRow(0);
            playFile(ui->playlistWidget->item(0)->text());
        }
    }
}

void VideoPlayer::onOpenFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Выберите видеофайлы",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "Видео файлы (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.mpg *.mpeg)");

    for (const QString &file : files) {
        addToPlaylist(file);
    }

    if (!files.isEmpty() && player->state() == QMediaPlayer::StoppedState) {
        playFile(files.first());
    }
}

void VideoPlayer::onPlayPause()
{
    if (player->state() == QMediaPlayer::PlayingState) {
        player->pause();
    } else {
        if (player->mediaStatus() == QMediaPlayer::NoMedia) {
            if (ui->playlistWidget->count() > 0) {
                ui->playlistWidget->setCurrentRow(0);
                playFile(ui->playlistWidget->item(0)->text());
            }
        } else {
            player->play();
        }
    }
}

void VideoPlayer::onStop()
{
    player->stop();
}

void VideoPlayer::onVolumeChanged(int volume)
{
    player->setVolume(volume);
    ui->volumeLabel->setText(QString("Громкость: %1%").arg(volume));
}

void VideoPlayer::onPositionChanged(qint64 position)
{
    if (!ui->positionSlider->isSliderDown()) {
        ui->positionSlider->setValue(position);
    }
}

void VideoPlayer::onDurationChanged(qint64 duration)
{
    ui->positionSlider->setRange(0, duration);
}

void VideoPlayer::onSliderMoved(int position)
{
    player->setPosition(position);
}

void VideoPlayer::onPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    playFile(item->text());
}

void VideoPlayer::onDeleteFile()
{
    int currentRow = ui->playlistWidget->currentRow();
    if (currentRow >= 0) {
        QString filePath = ui->playlistWidget->item(currentRow)->text();

        if (QMessageBox::question(this, "Удаление файла",
            "Удалить файл из плейлиста?\n" + QFileInfo(filePath).fileName(),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

            delete ui->playlistWidget->takeItem(currentRow);

            // Если удаляемый файл воспроизводится, остановить воспроизведение
            if (filePath == currentFile) {
                player->stop();
                currentFile.clear();
            }

            updatePlaylistButtons();
        }
    }
}

void VideoPlayer::onCopyFile()
{
    int currentRow = ui->playlistWidget->currentRow();
    if (currentRow >= 0) {
        QString sourcePath = ui->playlistWidget->item(currentRow)->text();
        QFileInfo fileInfo(sourcePath);

        QString destPath = QFileDialog::getSaveFileName(this,
            "Копировать файл как",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
            "/" + fileInfo.fileName(),
            "Все файлы (*.*)");

        if (!destPath.isEmpty()) {
            if (QFile::copy(sourcePath, destPath)) {
                QMessageBox::information(this, "Копирование", "Файл успешно скопирован");
            } else {
                QMessageBox::warning(this, "Копирование", "Ошибка при копировании файла");
            }
        }
    }
}

void VideoPlayer::onClearPlaylist()
{
    if (QMessageBox::question(this, "Очистка плейлиста",
        "Очистить весь плейлист?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        ui->playlistWidget->clear();
        player->stop();
        currentFile.clear();
        updatePlaylistButtons();
    }
}

void VideoPlayer::onSavePlaylist()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить плейлист",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Плейлист (*.m3u);;Все файлы (*.*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            for (int i = 0; i < ui->playlistWidget->count(); ++i) {
                stream << ui->playlistWidget->item(i)->text() << "\n";
            }
            file.close();
        }
    }
}

void VideoPlayer::onLoadPlaylist()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Загрузить плейлист",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Плейлист (*.m3u);;Все файлы (*.*)");

    if (!fileName.isEmpty()) {
        ui->playlistWidget->clear();

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (!line.isEmpty() && QFile::exists(line)) {
                    addToPlaylist(line);
                }
            }
            file.close();
        }

        updatePlaylistButtons();
    }
}

void VideoPlayer::updateTimeDisplay()
{
    qint64 duration = player->duration();
    qint64 position = player->position();

    QTime durationTime(0, 0, 0);
    QTime positionTime(0, 0, 0);

    durationTime = durationTime.addMSecs(duration);
    positionTime = positionTime.addMSecs(position);

    QString timeFormat = durationTime.hour() > 0 ? "hh:mm:ss" : "mm:ss";

    ui->timeLabel->setText(
        positionTime.toString(timeFormat) + " / " +
        durationTime.toString(timeFormat));
}

void VideoPlayer::addToPlaylist(const QString &filePath)
{
    if (QFile::exists(filePath)) {
        // Проверяем, нет ли уже этого файла в плейлисте
        for (int i = 0; i < ui->playlistWidget->count(); ++i) {
            if (ui->playlistWidget->item(i)->text() == filePath) {
                return;
            }
        }

        QListWidgetItem *item = new QListWidgetItem(
            QIcon(":/icons/video_icon.png"),
            QFileInfo(filePath).fileName());
        item->setText(filePath);
        item->setToolTip(filePath);
        ui->playlistWidget->addItem(item);

        updatePlaylistButtons();
    }
}

void VideoPlayer::playFile(const QString &filePath)
{
    if (QFile::exists(filePath)) {
        player->setMedia(QUrl::fromLocalFile(filePath));
        player->play();
        currentFile = filePath;

        // Подсвечиваем текущий файл в плейлисте
        for (int i = 0; i < ui->playlistWidget->count(); ++i) {
            if (ui->playlistWidget->item(i)->text() == filePath) {
                ui->playlistWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void VideoPlayer::updatePlaylistButtons()
{
    bool hasItems = ui->playlistWidget->count() > 0;
    bool hasSelection = ui->playlistWidget->currentRow() >= 0;

    ui->deleteFileButton->setEnabled(hasSelection);
    ui->copyFileButton->setEnabled(hasSelection);
    ui->clearPlaylistButton->setEnabled(hasItems);
    ui->savePlaylistButton->setEnabled(hasItems);
}
