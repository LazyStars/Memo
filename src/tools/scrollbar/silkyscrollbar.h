#pragma once

#include <qcolor.h>
#include <qeasingcurve.h>
#include <qpointer.h>
#include <qscrollbar.h>

class QAbstractScrollArea;
class QEvent;
class QPropertyAnimation;
class QWheelEvent;

class SilkyScrollBar : public QScrollBar
{
public:
    struct ScrollOptions {
        int wheelStep = 84;                         // 标准滚轮每个刻度对应的像素滚动距离
        qreal pixelDeltaFactor = 1.0;              // 触控板像素滚动的缩放系数
        int animationDurationMs = 180;             // 滚动动画时长，设为 0 可关闭动画
        QEasingCurve::Type easingCurve = QEasingCurve::OutCubic; // 滚动缓动曲线
        int minimumHandleLength = 36;              // 滑块最小尺寸，保证可拖拽性
        int thickness = 10;                        // 滚动条厚度
        int margin = 4;                            // 滚动条与内容区域的留白
        int radius = 5;                            // 轨道与滑块圆角
        QColor grooveColor = QColor(255, 255, 255, 12);         // 轨道底色
        QColor handleColor = QColor(170, 180, 200, 110);        // 默认滑块颜色
        QColor handleHoverColor = QColor(180, 205, 255, 150);   // 悬停滑块颜色
        QColor handlePressedColor = QColor(180, 205, 255, 190); // 按下滑块颜色
        QColor handleBorderColor = QColor(255, 255, 255, 26);   // 滑块描边颜色
    };

    explicit SilkyScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~SilkyScrollBar() override;

    void attachTo(QAbstractScrollArea* scrollArea);

    void setScrollOptions(const ScrollOptions& options);
    const ScrollOptions& scrollOptions() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void applyStyle();
    void animateToValue(int targetValue);
    int boundedValue(int value) const;
    int wheelOffsetFromEvent(const QWheelEvent* event) const;
    bool handleViewportWheelEvent(QWheelEvent* event);

private:
    QPointer<QAbstractScrollArea> attachedScrollArea;
    QPropertyAnimation* scrollAnimation = nullptr;
    ScrollOptions options;
};
