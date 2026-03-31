#pragma once

#include "TrainingClientBridge.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>

namespace syncdemo {

class TestInfoCanvas;

// MainWindow owns the visual layout: sync canvas, file-send controls,
// informational labels, and the bridge that talks to gdbus-learn.
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QString title, QWidget* parent = nullptr);

private slots:
    void applyInfo(const training::public_api::TestInfo& info);
    void showBackendError(const QString& message);
    void updateOperationStatus(const QString& message);
    void updateSelectedFile(const QString& path, bool has_selection);
    void showFileTransferResult(bool success, const QString& message);
    void chooseFile();
    void sendSelectedFile();

private:
    QLabel* title_label_{nullptr};
    QLabel* info_label_{nullptr};
    QLabel* file_transfer_label_{nullptr};
    QLabel* status_label_{nullptr};
    QLineEdit* file_path_edit_{nullptr};
    QPushButton* send_button_{nullptr};
    TestInfoCanvas* canvas_{nullptr};
    TrainingClientBridge* bridge_{nullptr};
};

} // namespace syncdemo
