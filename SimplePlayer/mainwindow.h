#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QListWidget>

#include "clickableslider.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    // Управление видео
    void onPlayPause();
    void onStop();
    void onOpenFile();

    // Управление звуком
    void onVolumeChanged(int volume);
    void onMuteToggled(bool muted);

    // Управление позицией
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSliderPressed();
    void onSliderReleased();
    void onSliderMoved(int position);

    // Управление плейлистом
    void onAddToPlaylist();
    void onDeleteFromPlaylist();
    void onClearPlaylist();
    void onCopyFile();
    void onFileProperties();
    void onSavePlaylist();
    void onLoadPlaylist();
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onPlaylistContextMenu(const QPoint &pos);

    // Навигация
    void onNextTrack();
    void onPrevTrack();

    // Меню
    void onTogglePlaylist(bool visible);
    void onToggleFullScreen(bool fullscreen);
    void onToggleLoopPlaylist(bool enabled);
    void onToggleLoopSingle(bool enabled);

    // Обновление UI
    void updateTimeDisplay();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void updateWindowTitle();
    void updatePlaybackSpeed();

private:
    Ui::MainWindow *ui;
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;

    QString m_currentFile;
    bool m_playlistLoop;
    bool m_singleFileLoop;
    bool m_isFullScreen;
    QByteArray m_savedGeometry;
    qreal m_playbackRate;
    ClickableSlider *newSlider;

    void setupPlayer();
    void setupConnections();
    void setupContextMenu();
    void addFileToPlaylist(const QString &filePath);
    void playFile(const QString &filePath);
    int findCurrentPlaylistIndex() const;
    void playNextTrack();
    void playPrevTrack();
    void updatePlaylistControls();
    QString formatFileSize(qint64 bytes);
    void loadSettings();
    void saveSettings();
};

#endif // MAINWINDOW_H
