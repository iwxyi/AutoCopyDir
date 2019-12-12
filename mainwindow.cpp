#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , prev_sync_timer(0), upload_time(0), download_time(0), syncing(false)
    , start_time(0)
{
    ui->setupUi(this);

    se = new QSettings("settings.ini");
    local_dir = se->value("local_dir").toString();
    remote_dir = se->value("remote_dir").toString();
    ui->lineEdit->setText(local_dir);
    ui->lineEdit_2->setText(remote_dir);
    int interval = se->value("timer_interval", 10).toInt();

    timer = new QTimer(this);
//    timer->setInterval(interval * 1000);
    connect(timer, &QTimer::timeout, this, [=]{
        if (ui->checkBox->isChecked())
            upload();
        if (ui->checkBox_2->isChecked())
            download();
        start_time = getTimestamp();
    });
//    timer->start();
//    start_time = getTimestamp();
    ui->spinBox->setValue(interval); // 通过这个来设置 interval 和 start

    progress_timer = new QTimer(this);
    progress_timer->setInterval(30);
    connect(progress_timer, &QTimer::timeout, this, [=]{
        if (timer->interval() == 0 || !timer->isActive()) // 没有进行同步
            return ;
        int prop = (getTimestamp() - start_time) * ui->progressBar->maximum() / timer->interval();
        ui->progressBar->setValue(prop);
    });
    progress_timer->start();

    upload_time = se->value("upload_time", 0).toLongLong();
    download_time = se->value("download_time", 0).toLongLong();
    ui->checkBox->setChecked(se->value("timer_upload", false).toBool());
    ui->checkBox_2->setChecked(se->value("timer_download", false).toBool());
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::copyDir(const QString &source, const QString &destination, bool override, qint64 filter_time)
{
    QDir directory(source);
    if (!directory.exists())
    {
        return false;
    }
    QString dir_name = directory.dirName();
    if (!dir_name.endsWith("/"))
        dir_name += "/";
    if (isIgnore(directory.dirName()))
        return false;

    QString srcPath = QDir::toNativeSeparators(source);
    if (!srcPath.endsWith(QDir::separator()))
        srcPath += QDir::separator();
    QString dstPath = QDir::toNativeSeparators(destination);
    if (!dstPath.endsWith(QDir::separator()))
        dstPath += QDir::separator();

    QDir root_dir(destination);
    root_dir.mkpath(destination);

    bool error = false;
    QStringList fileNames = directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    for (QStringList::size_type i=0; i != fileNames.size(); ++i)
    {
        QString fileName = fileNames.at(i);
        QString srcFilePath = srcPath + fileName;
        QString dstFilePath = dstPath + fileName;
        QFileInfo fileInfo(srcFilePath);
        if (fileInfo.isFile() || fileInfo.isSymLink())
        {
            if (fileInfo.lastModified().toMSecsSinceEpoch() <= filter_time || isIgnore(fileInfo.fileName()))
                continue;
            if (override)
            {
                QFile::setPermissions(dstFilePath, QFile::WriteOwner);
            }
            QFile::copy(srcFilePath, dstFilePath);
            qDebug() << "    复制文件：" << srcFilePath;
        }
        else if (fileInfo.isDir())
        {
            QDir dstDir(dstFilePath);
            dstDir.mkpath(dstFilePath);
            if (!copyDir(srcFilePath, dstFilePath, override, filter_time))
            {
                error = true;
            }
        }
    }

    return !error;
}

qint64 MainWindow::getTimestamp()
{
    return QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::upload()
{
    if (local_dir.isEmpty() || remote_dir.isEmpty())
        return ;
    if (syncing)
    {
        qDebug() << "正在同步中，请稍后再试";
        return ;
    }
    QtConcurrent::run([=]{
        syncing = true;
        qDebug() << "开始上传" << getTimestamp();
        copyDir(local_dir, remote_dir, true, upload_time);
        qDebug() << "上传成功" << (upload_time = getTimestamp());
        se->setValue("upload_time", upload_time);
        syncing = false;
    });
}

void MainWindow::download()
{
    if (local_dir.isEmpty() || remote_dir.isEmpty())
        return ;
    if (syncing)
    {
        qDebug() << "正在同步中，请稍后再试";
        return ;
    }
    QtConcurrent::run([=]{
        syncing = true;
        qDebug() << "开始下载" << getTimestamp();
        copyDir(remote_dir, local_dir, true, download_time);
        qDebug() << "下载成功" << (download_time = getTimestamp());
        se->setValue("download_time", download_time);
        syncing = false;
    });
}

bool MainWindow::isIgnore(QString name)
{
    if (ui->lineEdit_3->text().trimmed().isEmpty())
        return true;
    QRegExp re(ui->lineEdit_3->text());
    return !re.exactMatch(name);
}

void MainWindow::on_pushButton_clicked()
{
    upload();
}

void MainWindow::on_pushButton_2_clicked()
{
    download();
}

void MainWindow::on_lineEdit_editingFinished()
{
    local_dir = ui->lineEdit->text();
    se->setValue("local_dir", local_dir);
}

void MainWindow::on_lineEdit_2_editingFinished()
{
    remote_dir = ui->lineEdit_2->text();
    se->setValue("remote_dir", remote_dir);
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    se->setValue("timer_upload", ui->checkBox->isChecked());
    if (!ui->checkBox->isChecked() && !ui->checkBox_2->isChecked() && timer->isActive())
    {
        timer->stop();
    }
    else if ((ui->checkBox->isChecked() || ui->checkBox_2->isChecked()) && !timer->isActive())
    {
        timer->start();
        start_time = getTimestamp();
    }
}

void MainWindow::on_checkBox_2_stateChanged(int arg1)
{
    se->setValue("timer_download", ui->checkBox_2->isChecked());
    if (!ui->checkBox->isChecked() && !ui->checkBox_2->isChecked() && timer->isActive())
    {
        timer->stop();
    }
    else if ((ui->checkBox->isChecked() || ui->checkBox_2->isChecked()) && !timer->isActive())
    {
        timer->start();
        start_time = getTimestamp();
    }
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    int x = ui->spinBox->value();
    if (x < 0)
        x = 0;
    timer->setInterval(x*1000);
    if (x == 0 && timer->isActive())
        timer->stop();
    else if (x > 0 && !timer->isActive())
    {
        timer->start();
        start_time = getTimestamp();
    }
    se->setValue("timer_interval", x);
}
