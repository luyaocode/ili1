#ifndef REMINDER_H
#define REMINDER_H

#include <QDialog>
#include <QKeyEvent>

class FullscreenPopup : public QDialog
{
    Q_OBJECT
public:
    FullscreenPopup(QWidget *parent = nullptr);
private slots:
    void onCloseClicked();
protected:
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // REMINDER_H
