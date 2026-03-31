#include "TestInfoCanvas.hpp"

#include "SyncMath.hpp"

#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>

namespace syncdemo {

TestInfoCanvas::TestInfoCanvas(QWidget* parent)
    : QWidget(parent)
    , checkbox_(new QCheckBox(tr("Show input"), this))
    , line_edit_(new QLineEdit(this)) {
    setMinimumSize(480, 320);

    checkbox_->setText(tr("bool_param"));
    // Dragging is handled via the line edit event filter so text editing still
    // uses the standard widget behavior.
    line_edit_->installEventFilter(this);

    connect(checkbox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (is_applying_remote_update_) {
            return;
        }
        emit checkStateEdited(checked);
    });

    connect(line_edit_, &QLineEdit::textEdited, this, [this](const QString& text) {
        if (is_applying_remote_update_) {
            return;
        }
        emit textEditedByUser(text);
    });
}

// Remote updates are applied without re-emitting user-edit signals.
void TestInfoCanvas::applyInfo(const training::public_api::TestInfo& info) {
    is_applying_remote_update_ = true;
    info_ = info;
    checkbox_->setChecked(info_.bool_param);
    line_edit_->setVisible(info_.bool_param);
    line_edit_->setText(QString::fromStdString(info_.string_param));
    updateGeometryFromState();
    is_applying_remote_update_ = false;
}

// Keep the input box aligned when the parent window is resized.
void TestInfoCanvas::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateGeometryFromState();
}

// Treat clicks on the line edit frame as drag gestures, while preserving the
// editable interior for normal text input.
bool TestInfoCanvas::eventFilter(QObject* watched, QEvent* event) {
    if (watched != line_edit_) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouse_event = static_cast<QMouseEvent*>(event);
        const QRect editable_zone = line_edit_->rect().adjusted(8, 8, -8, -8);
        const bool drag_press = mouse_event->button() == Qt::LeftButton &&
                                !editable_zone.contains(mouse_event->position().toPoint());
        if (drag_press) {
            dragging_ = true;
            drag_press_offset_ = mouse_event->pos();
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove && dragging_) {
        auto* mouse_event = static_cast<QMouseEvent*>(event);
        const QPoint top_left = line_edit_->mapToParent(mouse_event->pos() - drag_press_offset_);
        const double logical_x = math::PixelToLogicalX(static_cast<double>(top_left.x()), width(), height());
        const auto parts = math::SplitPreviewCoordinate(logical_x);
        info_.int_param = parts.integer_part;
        info_.double_param = parts.fractional_part;
        updateGeometryFromState();
        emit dragPreviewRequested(logical_x);
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && dragging_) {
        auto* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::LeftButton) {
            dragging_ = false;
            const QPoint top_left = line_edit_->mapToParent(mouse_event->pos() - drag_press_offset_);
            const double logical_x = math::PixelToLogicalX(static_cast<double>(top_left.x()), width(), height());
            const auto parts = math::SplitCommittedCoordinate(logical_x);
            info_.int_param = parts.integer_part;
            info_.double_param = parts.fractional_part;
            updateGeometryFromState();
            emit dragCommitted(math::ComposeCoordinate(parts));
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

// Geometry comes from shared logical coordinates, not absolute pixels.
void TestInfoCanvas::updateGeometryFromState() {
    const double scale = math::ScaleFactor(width(), height());

    checkbox_->setGeometry(
        static_cast<int>(math::kBaseMargin * scale),
        static_cast<int>(math::kBaseMargin * scale),
        static_cast<int>(180 * scale),
        static_cast<int>(32 * scale));

    const int edit_width = static_cast<int>(math::kBaseEditWidth * scale);
    const int edit_height = static_cast<int>(math::kBaseEditHeight * scale);
    const int left = static_cast<int>(math::LogicalToPixelX(currentLogicalX(), width(), height()));
    const int top = static_cast<int>(math::kBaseTop * scale);
    line_edit_->setGeometry(left, top, edit_width, edit_height);
}

// The current logical coordinate is reconstructed from int + double parts.
double TestInfoCanvas::currentLogicalX() const {
    return math::ComposeCoordinate({info_.int_param, info_.double_param});
}

} // namespace syncdemo
