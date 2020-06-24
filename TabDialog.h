#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <QDialog>

namespace Ui {
class TabDialog;
}

class TabDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TabDialog(QList<QPair<QString, QWidget*>> widgets, QWidget *parent = 0);
    ~TabDialog();

private:
    Ui::TabDialog *ui;
};

#endif // TABDIALOG_H
