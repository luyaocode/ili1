#include "TextViewer.h"

TextViewer::TextViewer(QWidget *parent)
    : QWidget(parent)
{
    initUi();
}

void TextViewer::initUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setFontFamily("Monospace");
    layout->addWidget(m_textEdit);
}

void TextViewer::loadTextFile(const QString& filePath)
{
    clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "警告", "无法打开文件：" + file.errorString());
        m_textEdit->setText("无法打开文件：" + filePath);
        return;
    }

    // 自动检测文件编码（优先UTF-8，其次GBK）
    QString encoding = detectFileEncoding(filePath);
    QTextCodec* codec = QTextCodec::codecForName(encoding.toUtf8());
    if (!codec) codec = QTextCodec::codecForLocale();

    QByteArray data = file.readAll();
    QString text = codec->toUnicode(data);

    // 如果是不支持的二进制文件，显示提示
    if (text.contains(QChar::ReplacementCharacter) && data.size() > 1024)
    {
        m_textEdit->setText("不支持的文件格式（二进制文件）：" + filePath);
    }
    else
    {
        m_textEdit->setText(text);
    }

    file.close();
}

QString TextViewer::detectFileEncoding(const QString& filePath)
{
    // 简单编码检测：先尝试UTF-8，再尝试GBK
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return "UTF-8";

    QByteArray data = file.read(1024);
    file.close();

    // UTF-8 BOM检测
    if (data.startsWith("\xEF\xBB\xBF")) return "UTF-8";

    // 尝试UTF-8解码
    QString utf8Text = QString::fromUtf8(data);
    if (!utf8Text.contains(QChar::ReplacementCharacter)) return "UTF-8";

    // 尝试GBK解码
    QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
    QString gbkText = gbkCodec->toUnicode(data);
    if (!gbkText.contains(QChar::ReplacementCharacter)) return "GBK";

    return "UTF-8"; // 默认UTF-8
}

void TextViewer::clear()
{
    m_textEdit->clear();
}
