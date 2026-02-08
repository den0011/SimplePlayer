#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTime>
#include <QToolTip>
#include <QSettings>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QTextStream>
#include <QTimer>
#include <QStyle>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_player(new QMediaPlayer(this))
    , m_videoWidget(new QVideoWidget(this))
    , m_playlistLoop(false)
    , m_singleFileLoop(false)
    , m_isFullScreen(false)
    , m_playbackRate(1.0)
{
    ui->setupUi(this);

    QSlider *oldSlider = ui->positionSlider;

    newSlider = new ClickableSlider(oldSlider->parentWidget());

//    connect(ui->positionSlider, &ClickableSlider::sliderClicked,
//            this, [this](int value) {
//                // преобразование позиции
//                player->setPosition(value);
//            });

    // копируем параметры
    newSlider->setOrientation(oldSlider->orientation());
    newSlider->setRange(oldSlider->minimum(), oldSlider->maximum());
    newSlider->setValue(oldSlider->value());
    newSlider->setEnabled(oldSlider->isEnabled());
    newSlider->setObjectName(oldSlider->objectName());

    // заменяем в layout
    if (auto layout = oldSlider->parentWidget()->layout()) {
        layout->replaceWidget(oldSlider, newSlider);
    }

    // удаляем старый
    oldSlider->deleteLater();

    // переназначаем указатель ui
    ui->positionSlider = newSlider;

    ui->videoContainer->layout()->setContentsMargins(0, 0, 0, 0);
    ui->videoContainer->layout()->setSpacing(0);
    ui->videoContainer->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );
    ui->videoContainer->setStyleSheet("background: black;");
    ui->videoContainer->setAutoFillBackground(false);


    // Настраиваем видеоплеер
    setupPlayer();

    // Настраиваем соединения
    setupConnections();

    // Настраиваем контекстное меню
    setupContextMenu();

    // Разрешаем drag and drop
    setAcceptDrops(true);
    ui->listPlaylist->setAcceptDrops(true);
    m_videoWidget->setAcceptDrops(true);

    // Сохраняем геометрию для восстановления из полноэкранного режима
    m_savedGeometry = saveGeometry();

    // Загружаем настройки
    loadSettings();

    // Обновляем UI
    updatePlaylistControls();
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupPlayer()
{
    // Настраиваем видео выход
    m_player->setVideoOutput(m_videoWidget);

    // Устанавливаем видеовиджет в контейнер
    ui->videoLayout->addWidget(m_videoWidget);

    // Настраиваем начальный объем
    m_player->setVolume(ui->sliderVolume->value());
    onVolumeChanged(ui->sliderVolume->value());

    // Настраиваем слайдер позиции
    ui->positionSlider->setRange(0, 0);

    // Устанавливаем иконки для кнопок
    ui->btnPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->btnStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->btnOpen->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->btnPrev->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->btnNext->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->btnAdd->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));

    // Настраиваем плейлист
    ui->listPlaylist->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listPlaylist->setDragDropMode(QAbstractItemView::DropOnly);
    ui->listPlaylist->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MainWindow::setupConnections()
{
    // Кнопки управления
    connect(ui->btnPlay, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(ui->btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(ui->btnNext, &QPushButton::clicked, this, &MainWindow::onNextTrack);
    connect(ui->btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevTrack);
    connect(ui->btnAdd, &QPushButton::clicked, this, &MainWindow::onAddToPlaylist);

    // Слайдеры
    connect(ui->positionSlider, &QSlider::sliderPressed, this, &MainWindow::onSliderPressed);
    connect(ui->positionSlider, &QSlider::sliderReleased, this, &MainWindow::onSliderReleased);
    connect(ui->positionSlider, &QSlider::sliderMoved, this, &MainWindow::onSliderMoved);
    connect(ui->sliderVolume, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(ui->btnMute, &QPushButton::toggled, this, &MainWindow::onMuteToggled);

    // Плейлист
    connect(ui->listPlaylist, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(ui->listPlaylist, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onPlaylistContextMenu);

    // Кнопки плейлиста
    connect(ui->btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteFromPlaylist);
    connect(ui->btnClear, &QPushButton::clicked, this, &MainWindow::onClearPlaylist);
    connect(ui->btnCopy, &QPushButton::clicked, this, &MainWindow::onCopyFile);
    connect(ui->btnProperties, &QPushButton::clicked, this, &MainWindow::onFileProperties);
    connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::onSavePlaylist);
    connect(ui->btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadPlaylist);

    // Медиаплеер
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);

    connect(m_player, &QMediaPlayer::stateChanged, [this](QMediaPlayer::State state) {
        // Обновляем иконку кнопки воспроизведения
        ui->btnPlay->setIcon(style()->standardIcon(
            state == QMediaPlayer::PlayingState ?
            QStyle::SP_MediaPause : QStyle::SP_MediaPlay));

        // Обновляем заголовок окна
        updateWindowTitle();
    });

    // Меню
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(ui->actionPlay, &QAction::triggered, this, &MainWindow::onPlayPause);
    connect(ui->actionStop, &QAction::triggered, this, &MainWindow::onStop);
    connect(ui->actionNext, &QAction::triggered, this, &MainWindow::onNextTrack);
    connect(ui->actionPrev, &QAction::triggered, this, &MainWindow::onPrevTrack);
    connect(ui->actionShowPlaylist, &QAction::toggled, this, &MainWindow::onTogglePlaylist);
    connect(ui->actionFullScreen, &QAction::toggled, this, &MainWindow::onToggleFullScreen);
    connect(ui->actionLoopPlaylist, &QAction::toggled, this, &MainWindow::onToggleLoopPlaylist);
    connect(ui->actionLoopSingle, &QAction::toggled, this, &MainWindow::onToggleLoopSingle);
    connect(ui->actionSavePlaylist, &QAction::triggered, this, &MainWindow::onSavePlaylist);
    connect(ui->actionLoadPlaylist, &QAction::triggered, this, &MainWindow::onLoadPlaylist);
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);

    // Таймер для обновления времени
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTimeDisplay);
    timer->start(100);
}

void MainWindow::setupContextMenu()
{
    // Контекстное меню будет создаваться при каждом вызове
}

// ==== Drag & Drop ====
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        bool filesAdded = false;

        for (const QUrl &url : urlList) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                addFileToPlaylist(filePath);
                filesAdded = true;
            }
        }

        if (filesAdded && m_player->state() == QMediaPlayer::StoppedState &&
            ui->listPlaylist->count() > 0) {
            playFile(ui->listPlaylist->item(0)->text());
        }

        // Показываем плейлист, если он был скрыт
        if (filesAdded && !ui->dockPlaylist->isVisible()) {
            ui->dockPlaylist->setVisible(true);
            ui->actionShowPlaylist->setChecked(true);
        }
    }
}

// ==== Контекстное меню ====
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // Контекстное меню на видео
    if (m_videoWidget->geometry().contains(event->pos())) {
        QMenu menu(this);

        QAction *playAction = menu.addAction("▶ Воспроизвести");
        QAction *pauseAction = menu.addAction("⏸ Пауза");
        QAction *stopAction = menu.addAction("■ Стоп");
        menu.addSeparator();
        QAction *fullscreenAction = menu.addAction("⛶ Полноэкранный режим");
        fullscreenAction->setCheckable(true);
        fullscreenAction->setChecked(m_isFullScreen);

        connect(playAction, &QAction::triggered, this, &MainWindow::onPlayPause);
        connect(pauseAction, &QAction::triggered, this, &MainWindow::onPlayPause);
        connect(stopAction, &QAction::triggered, this, &MainWindow::onStop);
        connect(fullscreenAction, &QAction::triggered, [this]() {
            onToggleFullScreen(!m_isFullScreen);
        });

        menu.exec(event->globalPos());
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        // Обработка будет в contextMenuEvent
        event->accept();
    } else {
        QMainWindow::mousePressEvent(event);
    }
}

// ==== Управление видео ====
void MainWindow::onPlayPause()
{
    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        if (m_player->mediaStatus() == QMediaPlayer::NoMedia) {
            if (ui->listPlaylist->count() > 0) {
                int currentIndex = findCurrentPlaylistIndex();
                if (currentIndex >= 0) {
                    playFile(ui->listPlaylist->item(currentIndex)->text());
                } else {
                    playFile(ui->listPlaylist->item(0)->text());
                }
            }
        } else {
            m_player->play();
        }
    }
}

void MainWindow::onStop()
{
    m_player->stop();
}

void MainWindow::onOpenFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Выберите видеофайлы",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "Видео файлы (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.mpg *.mpeg *.webm *.m4v);;Все файлы (*.*)");

    bool filesAdded = false;
    for (const QString &file : files) {
        addFileToPlaylist(file);
        filesAdded = true;
    }

    if (filesAdded && m_player->state() == QMediaPlayer::StoppedState) {
        playFile(files.first());
    }

    // Показываем плейлист, если он был скрыт
    if (filesAdded && !ui->dockPlaylist->isVisible()) {
        ui->dockPlaylist->setVisible(true);
        ui->actionShowPlaylist->setChecked(true);
    }
}

void MainWindow::onAddToPlaylist()
{
    onOpenFile();
}

// ==== Управление звуком ====
void MainWindow::onVolumeChanged(int volume)
{
    m_player->setVolume(volume);

    // Обновляем иконку громкости
    if (volume == 0) {
        ui->lblVolumeIcon->setText("🔇");
    } else if (volume < 33) {
        ui->lblVolumeIcon->setText("🔈");
    } else if (volume < 66) {
        ui->lblVolumeIcon->setText("🔉");
    } else {
        ui->lblVolumeIcon->setText("🔊");
    }

    // Если громкость 0, включаем режим mute
    if (volume == 0) {
        ui->btnMute->setChecked(true);
    } else if (ui->btnMute->isChecked()) {
        ui->btnMute->setChecked(false);
    }

    // Обновляем подсказку
    ui->sliderVolume->setToolTip(QString("Громкость: %1%").arg(volume));
}

void MainWindow::onMuteToggled(bool muted)
{
    static int lastVolume = 50;

    if (muted) {
        lastVolume = m_player->volume();
        m_player->setVolume(0);
        ui->sliderVolume->setValue(0);
    } else {
        m_player->setVolume(lastVolume);
        ui->sliderVolume->setValue(lastVolume);
    }
}

// ==== Управление позицией ====
void MainWindow::onPositionChanged(qint64 position)
{
    if (!ui->positionSlider->isSliderDown()) {
        ui->positionSlider->setValue(position);
    }
}

void MainWindow::onDurationChanged(qint64 duration)
{
    ui->positionSlider->setRange(0, duration);
}

void MainWindow::onSliderPressed()
{
    // Временно отключаем обновление позиции от плеера
    disconnect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
}

void MainWindow::onSliderReleased()
{
    // Устанавливаем новую позицию
    m_player->setPosition(ui->positionSlider->value());

    // Восстанавливаем соединение
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
}

void MainWindow::onSliderMoved(int position)
{
    // Устанавливаем позицию при перемещении слайдера
    m_player->setPosition(position);

    // Показываем время под курсором
    QTime time(0, 0, 0);
    time = time.addMSecs(position);
    QToolTip::showText(QCursor::pos(), time.toString("hh:mm:ss"), ui->positionSlider);
}

// ==== Управление плейлистом ====
void MainWindow::addFileToPlaylist(const QString &filePath)
{
    if (QFile::exists(filePath)) {
        // Проверяем, нет ли уже этого файла в плейлисте
        for (int i = 0; i < ui->listPlaylist->count(); ++i) {
            if (ui->listPlaylist->item(i)->text() == filePath) {
                return;
            }
        }

        QListWidgetItem *item = new QListWidgetItem(
            style()->standardIcon(QStyle::SP_FileIcon),
            QFileInfo(filePath).fileName());
        item->setText(filePath);
        item->setToolTip(filePath);
        item->setData(Qt::UserRole, filePath);
        ui->listPlaylist->addItem(item);

        updatePlaylistControls();
    }
}

void MainWindow::onDeleteFromPlaylist()
{
    QList<QListWidgetItem*> selectedItems = ui->listPlaylist->selectedItems();
    if (selectedItems.isEmpty()) return;

    QString message;
    if (selectedItems.size() == 1) {
        message = "Удалить файл из плейлиста?\n" +
                 QFileInfo(selectedItems.first()->text()).fileName();
    } else {
        message = QString("Удалить %1 выбранных файлов из плейлиста?").arg(selectedItems.size());
    }

    if (QMessageBox::question(this, "Удаление файлов", message,
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        // Проверяем, не удаляем ли текущий воспроизводимый файл
        for (QListWidgetItem* item : selectedItems) {
            if (item->text() == m_currentFile) {
                m_player->stop();
                m_currentFile.clear();
                break;
            }
        }

        qDeleteAll(selectedItems);
        updatePlaylistControls();
    }
}

void MainWindow::onClearPlaylist()
{
    if (ui->listPlaylist->count() == 0) return;

    if (QMessageBox::question(this, "Очистка плейлиста",
        "Очистить весь плейлист?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        ui->listPlaylist->clear();
        m_player->stop();
        m_currentFile.clear();
        updatePlaylistControls();
    }
}

void MainWindow::onCopyFile()
{
    QListWidgetItem *item = ui->listPlaylist->currentItem();
    if (!item) return;

    QString sourcePath = item->text();
    QFileInfo fileInfo(sourcePath);

    QString destPath = QFileDialog::getSaveFileName(this,
        "Копировать файл как",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
        "/" + fileInfo.fileName(),
        QString("%1 (*.%2);;Все файлы (*.*)")
            .arg(fileInfo.suffix().toUpper())
            .arg(fileInfo.suffix()));

    if (!destPath.isEmpty()) {
        if (QFile::copy(sourcePath, destPath)) {
            QMessageBox::information(this, "Копирование", "Файл успешно скопирован");
        } else {
            QMessageBox::warning(this, "Копирование", "Ошибка при копировании файла");
        }
    }
}

void MainWindow::onFileProperties()
{
    QListWidgetItem *item = ui->listPlaylist->currentItem();
    if (!item) return;

    QString filePath = item->text();
    QFileInfo fileInfo(filePath);

    QString properties = QString(
        "<b>Свойства файла:</b><br><br>"
        "<b>Имя:</b> %1<br>"
        "<b>Расширение:</b> %2<br>"
        "<b>Размер:</b> %3<br>"
        "<b>Дата создания:</b> %4<br>"
        "<b>Дата изменения:</b> %5<br>"
        "<b>Путь:</b> %6<br>")
        .arg(fileInfo.fileName())
        .arg(fileInfo.suffix().toUpper())
        .arg(formatFileSize(fileInfo.size()))
        .arg(fileInfo.birthTime().toString("dd.MM.yyyy HH:mm:ss"))
        .arg(fileInfo.lastModified().toString("dd.MM.yyyy HH:mm:ss"))
        .arg(fileInfo.absolutePath());

    QMessageBox::information(this, "Свойства файла", properties);
}

QString MainWindow::formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 ГБ").arg(bytes / (double)GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 МБ").arg(bytes / (double)MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 КБ").arg(bytes / (double)KB, 0, 'f', 2);
    } else {
        return QString("%1 байт").arg(bytes);
    }
}

void MainWindow::onSavePlaylist()
{
    if (ui->listPlaylist->count() == 0) {
        QMessageBox::warning(this, "Сохранение плейлиста", "Плейлист пуст");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить плейлист",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/playlist.m3u",
        "Плейлист (*.m3u);;Текстовый файл (*.txt);;Все файлы (*.*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << "#EXTM3U\n";

            for (int i = 0; i < ui->listPlaylist->count(); ++i) {
                QString filePath = ui->listPlaylist->item(i)->text();
                QFileInfo fileInfo(filePath);
                stream << "#EXTINF:-1," << fileInfo.fileName() << "\n";
                stream << filePath << "\n";
            }

            file.close();
            QMessageBox::information(this, "Сохранение плейлиста", "Плейлист успешно сохранен");
        } else {
            QMessageBox::warning(this, "Сохранение плейлиста", "Ошибка при сохранении плейлиста");
        }
    }
}

void MainWindow::onLoadPlaylist()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Загрузить плейлист",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Плейлист (*.m3u *.pls);;Текстовый файл (*.txt);;Все файлы (*.*)");

    if (!fileName.isEmpty()) {
        ui->listPlaylist->clear();

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();

                // Пропускаем комментарии и заголовки M3U
                if (line.startsWith("#") || line.isEmpty()) {
                    continue;
                }

                // Проверяем, существует ли файл
                if (QFile::exists(line)) {
                    addFileToPlaylist(line);
                } else {
                    // Пробуем относительный путь
                    QFileInfo fileInfo(fileName);
                    QString absolutePath = fileInfo.absoluteDir().absoluteFilePath(line);
                    if (QFile::exists(absolutePath)) {
                        addFileToPlaylist(absolutePath);
                    }
                }
            }

            file.close();

            // Показываем плейлист
            if (!ui->dockPlaylist->isVisible() && ui->listPlaylist->count() > 0) {
                ui->dockPlaylist->setVisible(true);
                ui->actionShowPlaylist->setChecked(true);
            }

            updatePlaylistControls();
        } else {
            QMessageBox::warning(this, "Загрузка плейлиста", "Ошибка при загрузке плейлиста");
        }
    }
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    playFile(item->text());
}

void MainWindow::onPlaylistContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->listPlaylist->itemAt(pos);

    QMenu menu(this);

    if (item) {
        QAction *playAction = menu.addAction("▶ Воспроизвести");
        playAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

        QAction *deleteAction = menu.addAction("🗑 Удалить");
        deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

        QAction *copyAction = menu.addAction("📋 Копировать файл...");
        copyAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));

        QAction *propertiesAction = menu.addAction("📄 Свойства файла...");
        propertiesAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));

        menu.addSeparator();

        connect(playAction, &QAction::triggered, [this, item]() {
            playFile(item->text());
        });
        connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteFromPlaylist);
        connect(copyAction, &QAction::triggered, this, &MainWindow::onCopyFile);
        connect(propertiesAction, &QAction::triggered, this, &MainWindow::onFileProperties);
    }

    QAction *clearAction = menu.addAction("🗑 Очистить плейлист");
    clearAction->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearPlaylist);

    menu.addSeparator();

    QAction *addAction = menu.addAction("＋ Добавить файлы...");
    addAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    connect(addAction, &QAction::triggered, this, &MainWindow::onAddToPlaylist);

    menu.exec(ui->listPlaylist->viewport()->mapToGlobal(pos));
}

// ==== Навигация ====
void MainWindow::onNextTrack()
{
    playNextTrack();
}

void MainWindow::onPrevTrack()
{
    playPrevTrack();
}

void MainWindow::playNextTrack()
{
    int currentIndex = findCurrentPlaylistIndex();
    if (currentIndex >= 0) {
        int nextIndex = currentIndex + 1;
        if (nextIndex >= ui->listPlaylist->count()) {
            if (m_playlistLoop) {
                nextIndex = 0;
            } else {
                return;
            }
        }
        ui->listPlaylist->setCurrentRow(nextIndex);
        playFile(ui->listPlaylist->item(nextIndex)->text());
    } else if (ui->listPlaylist->count() > 0) {
        ui->listPlaylist->setCurrentRow(0);
        playFile(ui->listPlaylist->item(0)->text());
    }
}

void MainWindow::playPrevTrack()
{
    int currentIndex = findCurrentPlaylistIndex();
    if (currentIndex >= 0) {
        int prevIndex = currentIndex - 1;
        if (prevIndex < 0) {
            if (m_playlistLoop) {
                prevIndex = ui->listPlaylist->count() - 1;
            } else {
                return;
            }
        }
        ui->listPlaylist->setCurrentRow(prevIndex);
        playFile(ui->listPlaylist->item(prevIndex)->text());
    } else if (ui->listPlaylist->count() > 0) {
        ui->listPlaylist->setCurrentRow(ui->listPlaylist->count() - 1);
        playFile(ui->listPlaylist->item(ui->listPlaylist->count() - 1)->text());
    }
}

int MainWindow::findCurrentPlaylistIndex() const
{
    for (int i = 0; i < ui->listPlaylist->count(); ++i) {
        if (ui->listPlaylist->item(i)->text() == m_currentFile) {
            return i;
        }
    }
    return -1;
}

// ==== Меню ====
void MainWindow::onTogglePlaylist(bool visible)
{
    ui->dockPlaylist->setVisible(visible);
}

void MainWindow::onToggleFullScreen(bool fullscreen)
{
    if (fullscreen) {
        m_isFullScreen = true;
        menuBar()->hide();
        statusBar()->hide();
        ui->controlFrame->hide();
        ui->dockPlaylist->hide();
        showFullScreen();
    } else {
        m_isFullScreen = false;
        menuBar()->show();
        statusBar()->show();
        ui->controlFrame->show();
        if (ui->actionShowPlaylist->isChecked()) {
            ui->dockPlaylist->show();
        }
        showNormal();
        restoreGeometry(m_savedGeometry);
    }
}

void MainWindow::onToggleLoopPlaylist(bool enabled)
{
    m_playlistLoop = enabled;
    if (enabled) {
        m_singleFileLoop = false;
        ui->actionLoopSingle->setChecked(false);
    }
}

void MainWindow::onToggleLoopSingle(bool enabled)
{
    m_singleFileLoop = enabled;
    if (enabled) {
        m_playlistLoop = false;
        ui->actionLoopPlaylist->setChecked(false);
    }
}

// ==== Обновление UI ====
void MainWindow::updateTimeDisplay()
{
    qint64 duration = m_player->duration();
    qint64 position = m_player->position();

    QTime durationTime(0, 0, 0);
    QTime positionTime(0, 0, 0);

    durationTime = durationTime.addMSecs(duration);
    positionTime = positionTime.addMSecs(position);

    QString timeFormat = durationTime.hour() > 0 ? "hh:mm:ss" : "mm:ss";

    ui->lblTime->setText(
        positionTime.toString(timeFormat) + " / " +
        durationTime.toString(timeFormat));
}

void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::EndOfMedia:
        if (m_singleFileLoop) {
            // Зацикливаем текущий файл
            m_player->setPosition(0);
            m_player->play();
        } else if (m_playlistLoop) {
            // Переходим к следующему файлу в плейлисте
            playNextTrack();
        } else {
            // Автоматически переходим к следующему треку
            playNextTrack();
        }
        break;
    case QMediaPlayer::LoadedMedia:
        updateWindowTitle();
        break;
    case QMediaPlayer::InvalidMedia:
        QMessageBox::warning(this, "Ошибка", "Невозможно воспроизвести файл: " + m_currentFile);
        break;
    default:
        break;
    }
}

void MainWindow::updateWindowTitle()
{
    if (!m_currentFile.isEmpty()) {
        QFileInfo fileInfo(m_currentFile);
        QString state;

        switch (m_player->state()) {
        case QMediaPlayer::PlayingState:
            state = "▶ ";
            break;
        case QMediaPlayer::PausedState:
            state = "⏸ ";
            break;
        case QMediaPlayer::StoppedState:
            state = "■ ";
            break;
        }

        setWindowTitle(state + fileInfo.fileName() + " - Видеоплеер");
    } else {
        setWindowTitle("Видеоплеер");
    }
}

void MainWindow::updatePlaybackSpeed()
{
    // Функция для будущей реализации изменения скорости воспроизведения
    // m_player->setPlaybackRate(m_playbackRate);
}

void MainWindow::playFile(const QString &filePath)
{
    if (QFile::exists(filePath)) {
        m_player->setMedia(QUrl::fromLocalFile(filePath));
        m_player->play();
        m_currentFile = filePath;

        // Подсвечиваем текущий файл в плейлисте
        for (int i = 0; i < ui->listPlaylist->count(); ++i) {
            if (ui->listPlaylist->item(i)->text() == filePath) {
                ui->listPlaylist->setCurrentRow(i);
                ui->listPlaylist->scrollToItem(ui->listPlaylist->item(i));
                break;
            }
        }

        updateWindowTitle();
    }
}

void MainWindow::updatePlaylistControls()
{
    bool hasItems = ui->listPlaylist->count() > 0;
    bool hasSelection = !ui->listPlaylist->selectedItems().isEmpty();
    bool singleSelection = ui->listPlaylist->selectedItems().size() == 1;

    ui->btnDelete->setEnabled(hasSelection);
    ui->btnCopy->setEnabled(singleSelection);
    ui->btnProperties->setEnabled(singleSelection);
    ui->btnClear->setEnabled(hasItems);
    ui->btnSave->setEnabled(hasItems);
    ui->btnLoad->setEnabled(true);
    ui->btnNext->setEnabled(hasItems);
    ui->btnPrev->setEnabled(hasItems);
    ui->btnPlay->setEnabled(hasItems);
    ui->btnStop->setEnabled(hasItems);
    ui->btnAdd->setEnabled(true);

    ui->actionPlay->setEnabled(hasItems);
    ui->actionStop->setEnabled(hasItems);
    ui->actionNext->setEnabled(hasItems);
    ui->actionPrev->setEnabled(hasItems);
    ui->actionSavePlaylist->setEnabled(hasItems);

    // Обновляем статусную строку
    if (hasItems) {
        ui->statusbar->showMessage(QString("Файлов в плейлисте: %1").arg(ui->listPlaylist->count()));
    } else {
        ui->statusbar->showMessage("Плейлист пуст");
    }
}

// ==== Клавиатура ====
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        onPlayPause();
        event->accept();
        break;
    case Qt::Key_Left:
        if (event->modifiers() & Qt::ControlModifier) {
            onPrevTrack();
        } else {
            m_player->setPosition(m_player->position() - 5000);
        }
        event->accept();
        break;
    case Qt::Key_Right:
        if (event->modifiers() & Qt::ControlModifier) {
            onNextTrack();
        } else {
            m_player->setPosition(m_player->position() + 5000);
        }
        event->accept();
        break;
    case Qt::Key_Up:
        m_player->setVolume(qMin(m_player->volume() + 5, 100));
        ui->sliderVolume->setValue(m_player->volume());
        event->accept();
        break;
    case Qt::Key_Down:
        m_player->setVolume(qMax(m_player->volume() - 5, 0));
        ui->sliderVolume->setValue(m_player->volume());
        event->accept();
        break;
    case Qt::Key_F:
        if (event->modifiers() & Qt::ControlModifier) {
            onToggleFullScreen(!m_isFullScreen);
            event->accept();
        }
        break;
    case Qt::Key_F11:
        onToggleFullScreen(!m_isFullScreen);
        event->accept();
        break;
    case Qt::Key_Escape:
        if (m_isFullScreen) {
            onToggleFullScreen(false);
            event->accept();
        }
        break;
    case Qt::Key_Delete:
        onDeleteFromPlaylist();
        event->accept();
        break;
    case Qt::Key_A:
        if (event->modifiers() & Qt::ControlModifier) {
            ui->listPlaylist->selectAll();
            event->accept();
        }
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_videoWidget->geometry().contains(event->pos())) {
            onToggleFullScreen(!m_isFullScreen);
        }
    }
    QMainWindow::mouseDoubleClickEvent(event);
}

// ==== Настройки ====
void MainWindow::saveSettings()
{
    QSettings settings("MyCompany", "VideoPlayer");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("volume", m_player->volume());
    settings.setValue("playlistVisible", ui->dockPlaylist->isVisible());
    settings.setValue("playlistLoop", m_playlistLoop);
    settings.setValue("singleFileLoop", m_singleFileLoop);

    // Сохраняем плейлист
    QStringList playlist;
    for (int i = 0; i < ui->listPlaylist->count(); ++i) {
        playlist << ui->listPlaylist->item(i)->text();
    }
    settings.setValue("playlist", playlist);
}

void MainWindow::loadSettings()
{
    QSettings settings("MyCompany", "VideoPlayer");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    int volume = settings.value("volume", 50).toInt();
    m_player->setVolume(volume);
    ui->sliderVolume->setValue(volume);
    onVolumeChanged(volume);

    bool playlistVisible = settings.value("playlistVisible", false).toBool();
    ui->dockPlaylist->setVisible(playlistVisible);
    ui->actionShowPlaylist->setChecked(playlistVisible);

    m_playlistLoop = settings.value("playlistLoop", false).toBool();
    ui->actionLoopPlaylist->setChecked(m_playlistLoop);

    m_singleFileLoop = settings.value("singleFileLoop", false).toBool();
    ui->actionLoopSingle->setChecked(m_singleFileLoop);

    // Загружаем плейлист
    QStringList playlist = settings.value("playlist").toStringList();
    for (const QString &filePath : playlist) {
        if (QFile::exists(filePath)) {
            addFileToPlaylist(filePath);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}
