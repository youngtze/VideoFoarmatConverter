#ifndef CONVERTTASK_H
#define CONVERTTASK_H

#include <QObject>
#include <QThread>

class ConvertTask : public QThread
{
    Q_OBJECT
public:
    explicit ConvertTask(QObject *parent = nullptr);
    void setInFilePath(QString inFilePath);
    void setOutFilePath(QString outFilePath);
    void run() override;

private:
    QString inFilePath;
    QString outFilePath;

signals:
    void notifyProgress(double progress);
};

#endif // CONVERTTASK_H
