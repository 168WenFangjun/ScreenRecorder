#pragma once

#include <QWidget>

namespace Ui {
class CrossCursorWidget;
}

class CrossCursorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CrossCursorWidget(const QString &title, const QString &label, const QImage &img, QWidget *parent = 0);
    ~CrossCursorWidget();
    void show(int x, int y);

signals:
    void done();
protected:

private slots:
    void on_pushButtonStartDrag_clicked();

private:
    Ui::CrossCursorWidget *ui;
};
