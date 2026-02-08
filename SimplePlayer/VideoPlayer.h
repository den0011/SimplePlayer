#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QFileDialog>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QMimeData>
#include <QFileInfo>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class VideoPlayer;
}
QT_END_NAMESPACE

class VideoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onOpenFile();
    void onPlayPause();
    void onStop();
    void onVolumeChanged(int volume);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSliderMoved(int position);
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onDeleteFile();
    void onCopyFile();
    void onClearPlaylist();
    void onSavePlaylist();
    void onLoadPlaylist();
    void updateTimeDisplay();

private:
    Ui::VideoPlayer *ui;
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QString currentFile;

    void setupUI();
    void setupConnections();
    void addToPlaylist(const QString &filePath);
    void playFile(const QString &filePath);
    void updatePlaylistButtons();
};
#endif // VIDEOPLAYER_H
