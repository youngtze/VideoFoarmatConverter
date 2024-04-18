#include "dialog.h"
#include "./ui_dialog.h"

#include <QFileDialog>
#include <iostream>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
}


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);

    convertTask = new ConvertTask();

    QObject::connect(ui->chooseFile, SIGNAL(clicked()), this, SLOT(chooseFile()));
    QObject::connect(ui->saveFile, SIGNAL(clicked()), this, SLOT(saveFile()));
    QObject::connect(ui->convert, SIGNAL(clicked()), this, SLOT(startConvert()));
    QObject::connect(convertTask, &ConvertTask::notifyProgress, this, &Dialog::updateProgress, Qt::QueuedConnection);
}

void Dialog::chooseFile() {
    this->inFilePath =  QFileDialog::getOpenFileName(this, tr("选择一个视文件"), "", "Video Files (*.mp4)");
    ui->inFile->setText(inFilePath);
    std::cout << "chooseFile, inFilePath=" << inFilePath.toStdString() << std::endl;
}


void Dialog::saveFile() {
    this->outFilePath = QFileDialog::getSaveFileName(this, tr("文件存储为"), tr(""), tr("FLV Video Files (*.flv);; AVI Video Files (*.avi)"));
    ui->outFile->setText(outFilePath);
    std::cout << "saveFile, outFilePath=" << outFilePath.toStdString() << std::endl;
}

void Dialog::updateProgress(double progress) {
    std::cout << "updateProgress(), progress=" << progress << ", current thread id = "<< QThread::currentThreadId() << std::endl;
    ui->progressBar->setValue(progress);
}

void Dialog::startConvert() {
    ui->progressBar->setVisible(true);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setTextVisible(true);
    // 进度条样式
    ui->progressBar->setStyleSheet("QProgressBar {"
                              " text-align: center;"
                              " color: blue;"
                              " font-size: 16px;"
                              " font-weight: bold;"
                              " border: 1px solid green;"
                              " border-radius: 1000px;"
                              " background-color: white;"
                              "}"
                              "QProgressBar::chunk {"
                              "  background-color: green;"
                              "  border-radius: 1000px;"
                              "  font: bold 14px;"
                              "}");

    updateProgress(10);
    convertTask->setInFilePath(inFilePath);
    convertTask->setOutFilePath(outFilePath);
    convertTask->start();
    std::cout <<"startConvert(), current thread id = "<< QThread::currentThreadId() << std::endl;
}


Dialog::~Dialog()
{
    delete ui;
}
