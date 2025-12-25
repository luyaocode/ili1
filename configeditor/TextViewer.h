#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QFile>
#include <QMessageBox>
#include <QTextCodec>

class TextViewer : public QWidget
{
    Q_OBJECT

public:
    TextViewer(QWidget *parent = nullptr);
    ~TextViewer() = default;

public slots:
    void loadTextFile(const QString& filePath);

private:
    void initUi();
    void clear();
    QString detectFileEncoding(const QString& filePath);

    QTextEdit* m_textEdit;
};

#endif // TEXTVIEWER_H
