#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "converttask.h"//

QT_BEGIN_NAMESPACE
namespace Ui {
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private:
    Ui::Dialog *ui;
    QString inFilePath;
    QString outFilePath;
    ConvertTask* convertTask;

private slots:
    void chooseFile();
    void saveFile();
    void startConvert();
public slots:
    void updateProgress(double progress);
};

#endif // DIALOG_H
