#ifndef ABOUTWIDGET_H
#define ABOUTWIDGET_H

#include <QWidget>

namespace Ui {
class AboutWidget;
}

class AboutWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AboutWidget(QWidget *parent = 0);
    ~AboutWidget();

private slots:
    void on_pushButtonEmail_clicked();
    void on_pushButtonUrl_clicked();

private:
    Ui::AboutWidget *ui;

    void makePushButtonIntoWebLink();
};

#endif // ABOUTWIDGET_H
