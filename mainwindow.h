#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_checkBox_stateChanged(int arg1);

    void on_checkBox_2_stateChanged(int arg1);

    void on_spinBox_valueChanged(int arg1);

    void on_pushButton_3_clicked();

    void on_lineEdit_3_textEdited(const QString &arg1);

private:
    bool copyDir(const QString &source, const QString &destination, bool override = false, qint64 filter_time = 0);
    qint64 getTimestamp();

    inline bool needSync();
    void startTimer();
    void stopTimer();

    void upload();
    void download();
    bool isIgnore(QString name);

private:
    Ui::MainWindow *ui;
    QTimer* timer;
    QSettings* se;
    qint64 prev_sync_timer, upload_time, download_time;
    bool syncing;

    QString local_dir, remote_dir;
    QStringList ignores;

    QTimer* progress_timer;
    qint64 start_time;
};
#endif // MAINWINDOW_H
