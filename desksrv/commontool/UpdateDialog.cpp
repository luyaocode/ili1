// UpdateDialog.cpp
#include "UpdateDialog.h"
#include <QFont>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonArray>

UpdateDialog::UpdateDialog(const QJsonObject& updateInfo, QWidget *parent)
    : QDialog(parent)
{
    // 设置弹窗属性
    setWindowTitle(updateInfo["updateTitle"].toString());
    setModal(true); // 模态弹窗，阻塞程序直到关闭
    setFixedSize(500, 400); // 固定大小，避免拉伸

    // 初始化UI
    initUI(updateInfo);
}

void UpdateDialog::initUI(const QJsonObject& updateInfo)
{
    // 1. 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // 2. 版本提示标签
    QLabel* versionLabel = new QLabel(this);
    QString versionText = QString("当前版本：%1  →  最新版本：%2")
                          .arg(updateInfo["currentVersion"].toString())
                          .arg(updateInfo["latestVersion"].toString());
    versionLabel->setText(versionText);
    QFont versionFont = versionLabel->font();
    versionFont.setBold(true);
    versionFont.setPointSize(14);
    versionLabel->setFont(versionFont);
    mainLayout->addWidget(versionLabel);

    // 3. 更新说明文本框（只读）
    QTextEdit* contentEdit = new QTextEdit(this);
    contentEdit->setReadOnly(true);
    contentEdit->setFontPointSize(12);
    // 拼接更新内容列表
    QString contentText;
    QJsonArray contentArray = updateInfo["updateContent"].toArray();
    for (int i = 0; i < contentArray.size(); i++) {
        contentText += contentArray[i].toString() + "\n";
    }
    contentEdit->setText(contentText);
    mainLayout->addWidget(contentEdit);

    // 4. 确认按钮
    QPushButton* confirmBtn = new QPushButton("确认", this);
    confirmBtn->setFixedSize(100, 40);
    connect(confirmBtn, &QPushButton::clicked, this, &UpdateDialog::accept);
    // 强制更新：隐藏关闭按钮，只能点确认
    if (updateInfo["forceUpdate"].toBool()) {
        setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    }

    // 按钮居中
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(confirmBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}
