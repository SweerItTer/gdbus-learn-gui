#pragma once

#include <public/DbusConstants.hpp>

#include <QCheckBox>
#include <QLineEdit>
#include <QWidget>

namespace syncdemo {

// TestInfoCanvas is the custom widget that visualizes one TestInfo object.
class TestInfoCanvas final : public QWidget {
    Q_OBJECT

public:
    explicit TestInfoCanvas(QWidget* parent = nullptr);

    void applyInfo(const training::public_api::TestInfo& info);

signals:
    // These signals describe user-originated changes only.
    void checkStateEdited(bool checked);
    void textEditedByUser(const QString& text);
    void dragPreviewRequested(double logical_x);
    void dragCommitted(double logical_x);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // Recompute child control geometry after resize or remote sync updates.
    void updateGeometryFromState();
    double currentLogicalX() const;

    QCheckBox* checkbox_{nullptr};
    QLineEdit* line_edit_{nullptr};
    training::public_api::TestInfo info_{};
    bool is_applying_remote_update_{false};
    bool dragging_{false};
    QPoint drag_press_offset_;
};

} // namespace syncdemo
