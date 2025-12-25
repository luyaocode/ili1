#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QTextEdit>
#include "imagedetector.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. 创建主窗口
    QWidget window;
    window.setWindowTitle("AI生成图片检测器（像素/频域特征版）");
    window.resize(800, 600);

    // 2. 创建控件
    QLabel *imageLabel = new QLabel("请选择图片...");
    imageLabel->setAlignment(Qt::AlignCenter);
    QPushButton *selectBtn  = new QPushButton("选择图片");
    QTextEdit   *resultEdit = new QTextEdit();
    resultEdit->setReadOnly(true);
    resultEdit->setPlaceholderText("检测结果将显示在这里...");

    // 3. 布局
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(imageLabel);
    layout->addWidget(selectBtn);
    layout->addWidget(resultEdit);

    // 4. 初始化检测器
    ImageDetector detector;

    // 5. 选中的图片
    QImage selectedImage;

    // 6. 选择图片槽函数
    QObject::connect(selectBtn, &QPushButton::clicked, [&]() {
        QString filePath =
            QFileDialog::getOpenFileName(&window, "选择图片", "", "Image Files (*.png *.jpg *.jpeg *.bmp)");
        if (!filePath.isEmpty())
        {
            selectedImage = QImage(filePath);
            imageLabel->setPixmap(QPixmap::fromImage(selectedImage).scaled(600, 400, Qt::KeepAspectRatio));

            // 执行检测
            DetectResult res = detector.detect(selectedImage);

            // 显示结果
            QString resultStr = QString("检测结果：%1\n"
                                        "综合评分：%2\n"
                                        "--- 分项特征 ---\n"
                                        "噪声规律性得分：%3\n"
                                        "梯度异常得分：%4\n"
                                        "频域规律得分：%5")
                                    .arg(res.isAIGenerated ? "AI生成图片" : "真实图片")
                                    .arg(res.score, 0, 'f', 4)
                                    .arg(res.noiseRandomScore, 0, 'f', 4)
                                    .arg(res.gradientAbnormScore, 0, 'f', 4)
                                    .arg(res.freqRegularScore, 0, 'f', 4);

            resultEdit->setText(resultStr);
        }
    });

    window.show();
    return a.exec();
}
