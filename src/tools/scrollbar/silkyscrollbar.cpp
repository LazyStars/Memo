#include "silkyscrollbar.h"

#include <qabstractscrollarea.h>
#include <qevent.h>
#include <qpropertyanimation.h>
#include <qstring.h>

SilkyScrollBar::SilkyScrollBar(Qt::Orientation orientation, QWidget* parent)
    : QScrollBar(orientation, parent) {
    scrollAnimation = new QPropertyAnimation(this, "value", this);
    scrollAnimation->setDuration(options.animationDurationMs);
    scrollAnimation->setEasingCurve(options.easingCurve);

    setContextMenuPolicy(Qt::NoContextMenu);
    setAttribute(Qt::WA_Hover, true);
    setSingleStep(qMax(1, options.wheelStep / 3));

    applyStyle();
}

SilkyScrollBar::~SilkyScrollBar() {
    attachTo(nullptr);
}

void SilkyScrollBar::attachTo(QAbstractScrollArea* scrollArea) {
    if (attachedScrollArea == scrollArea) {
        return;
    }

    if (attachedScrollArea != nullptr && attachedScrollArea->viewport() != nullptr) {
        attachedScrollArea->viewport()->removeEventFilter(this);
    }

    attachedScrollArea = scrollArea;

    if (attachedScrollArea != nullptr && attachedScrollArea->viewport() != nullptr) {
        attachedScrollArea->viewport()->installEventFilter(this);
    }
}

void SilkyScrollBar::setScrollOptions(const ScrollOptions& newOptions) {
    options = newOptions;
    scrollAnimation->setDuration(qMax(0, options.animationDurationMs));
    scrollAnimation->setEasingCurve(options.easingCurve);
    setSingleStep(qMax(1, options.wheelStep / 3));
    applyStyle();
}

const SilkyScrollBar::ScrollOptions& SilkyScrollBar::scrollOptions() const {
    return options;
}

bool SilkyScrollBar::eventFilter(QObject* watched, QEvent* event) {
    if (attachedScrollArea != nullptr
        && watched == attachedScrollArea->viewport()
        && event != nullptr) {
        if (event->type() == QEvent::Wheel) {
            return handleViewportWheelEvent(static_cast<QWheelEvent*>(event));
        }

        if (event->type() == QEvent::Destroy) {
            attachedScrollArea = nullptr;
        }
    }

    return QScrollBar::eventFilter(watched, event);
}

void SilkyScrollBar::applyStyle() {
    auto colorToRgba = [](const QColor& color) {
        return QStringLiteral("rgba(%1, %2, %3, %4)")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alpha());
    };

    const bool isVertical = orientation() == Qt::Vertical;
    const QString orientationName = isVertical ? QStringLiteral("vertical")
                                               : QStringLiteral("horizontal");
    const QString sizeRule = isVertical
                             ? QStringLiteral("width: %1px;").arg(options.thickness)
                             : QStringLiteral("height: %1px;").arg(options.thickness);
    const QString grooveMargin = isVertical
                                 ? QStringLiteral("margin: %1px 0px %1px 0px;").arg(options.margin)
                                 : QStringLiteral("margin: 0px %1px 0px %1px;").arg(options.margin);
    const QString handleSizeRule = isVertical
                                   ? QStringLiteral("min-height: %1px;").arg(options.minimumHandleLength)
                                   : QStringLiteral("min-width: %1px;").arg(options.minimumHandleLength);
    const QString zeroSizeRule = isVertical ? QStringLiteral("height: 0px;")
                                            : QStringLiteral("width: 0px;");

    QString styleSheet = QStringLiteral(
                             "QScrollBar:%1 {"
                             " background: transparent;"
                             " border: none;"
                             " margin: 0px;"
                             " %2"
                             "}"
                             "QScrollBar::groove:%1 {"
                             " background: %3;"
                             " border: none;"
                             " border-radius: %4px;"
                             " %2"
                             " %5"
                             "}"
                             "QScrollBar::handle:%1 {"
                             " background: %6;"
                             " border: 1px solid %7;"
                             " border-radius: %4px;"
                             " %8"
                             "}"
                             "QScrollBar::handle:%1:hover {"
                             " background: %9;"
                             "}"
                             "QScrollBar::handle:%1:pressed {"
                             " background: %10;"
                             "}"
                             "QScrollBar::add-line:%1, QScrollBar::sub-line:%1 {"
                             " background: transparent;"
                             " border: none;"
                             " %11"
                             "}"
                             "QScrollBar::add-page:%1, QScrollBar::sub-page:%1 {"
                             " background: transparent;"
                             "}");
    styleSheet = styleSheet.arg(orientationName);
    styleSheet = styleSheet.arg(sizeRule);
    styleSheet = styleSheet.arg(colorToRgba(options.grooveColor));
    styleSheet = styleSheet.arg(QString::number(options.radius));
    styleSheet = styleSheet.arg(grooveMargin);
    styleSheet = styleSheet.arg(colorToRgba(options.handleColor));
    styleSheet = styleSheet.arg(colorToRgba(options.handleBorderColor));
    styleSheet = styleSheet.arg(handleSizeRule);
    styleSheet = styleSheet.arg(colorToRgba(options.handleHoverColor));
    styleSheet = styleSheet.arg(colorToRgba(options.handlePressedColor));
    styleSheet = styleSheet.arg(zeroSizeRule);

    setStyleSheet(styleSheet);
}

void SilkyScrollBar::animateToValue(int targetValue) {
    const int boundedTargetValue = boundedValue(targetValue);
    const bool isAnimating = scrollAnimation->state() == QAbstractAnimation::Running;
    if (boundedTargetValue == value() && !isAnimating) {
        return;
    }

    scrollAnimation->stop();

    if (options.animationDurationMs <= 0) {
        setValue(boundedTargetValue);
        return;
    }

    if (boundedTargetValue == value()) {
        return;
    }

    scrollAnimation->setStartValue(value());
    scrollAnimation->setEndValue(boundedTargetValue);
    scrollAnimation->start();
}

int SilkyScrollBar::boundedValue(int value) const {
    return qBound(minimum(), value, maximum());
}

int SilkyScrollBar::wheelOffsetFromEvent(const QWheelEvent* event) const {
    if (event == nullptr) {
        return 0;
    }

    const bool isVertical = orientation() == Qt::Vertical;
    const int pixelDelta = isVertical ? event->pixelDelta().y() : event->pixelDelta().x();
    if (pixelDelta != 0) {
        return -qRound(pixelDelta * options.pixelDeltaFactor);
    }

    const int angleDelta = isVertical ? event->angleDelta().y() : event->angleDelta().x();
    if (angleDelta == 0) {
        return 0;
    }

    return -qRound((static_cast<qreal>(angleDelta) / 120.0) * options.wheelStep);
}

bool SilkyScrollBar::handleViewportWheelEvent(QWheelEvent* event) {
    if (event == nullptr
        || !isVisible()
        || !isEnabled()
        || minimum() >= maximum()
        || (event->modifiers() & Qt::ControlModifier) != 0) {
        return false;
    }

    const int offset = wheelOffsetFromEvent(event);
    if (offset == 0) {
        return false;
    }

    const int currentTargetValue = scrollAnimation->state() == QAbstractAnimation::Running
                                   ? scrollAnimation->endValue().toInt()
                                   : value();
    animateToValue(currentTargetValue + offset);
    event->accept();
    return true;
}
