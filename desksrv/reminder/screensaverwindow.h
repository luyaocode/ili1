#include <QApplication>
#include <QMainWindow>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QRect>
#include <QShowEvent>
#include <QScopedPointer>

class KeyBlocker;

// 自定义窗口类，用于展示图片或屏幕保护
class ScreenSaverWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScreenSaverWindow(const QString &imagePath, QWidget *parent = nullptr);
    ~ScreenSaverWindow();

protected:
    // 重绘事件
    void paintEvent(QPaintEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 更新动画状态
    void updateAnimation();

private:
    QScopedPointer<KeyBlocker> m_winBlocker;
    QString                    m_imagePath;  // 图片路径
    QPixmap                    m_pixmap;     // 图片对象
    bool                       m_hasImage;   // 是否有有效图片
    QTimer                    *m_timer;      // 动画定时器
    int                        m_angle;      // 旋转角度
};
