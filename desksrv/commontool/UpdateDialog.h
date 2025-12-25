// UpdateDialog.h
#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QJsonObject>

class UpdateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpdateDialog(const QJsonObject& updateInfo, QWidget *parent = nullptr);

private:
    // 初始化UI
    void initUI(const QJsonObject& updateInfo);
};

#endif // UPDATEDIALOG_H
