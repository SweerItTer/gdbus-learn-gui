#include "MainWindow.hpp"

#include "TestInfoCanvas.hpp"

#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

namespace syncdemo {

MainWindow::MainWindow(QString title, QWidget* parent)
    : QMainWindow(parent)
    , title_label_(new QLabel(std::move(title)))
    , info_label_(new QLabel(tr("Waiting for first state...")))
    , file_transfer_label_(new QLabel(tr("No file has been sent yet.")))
    , status_label_(new QLabel(tr("Connecting to system bus service...")))
    , file_path_edit_(new QLineEdit)
    , upload_remote_path_edit_(new QLineEdit)
    , download_remote_path_edit_(new QLineEdit)
    , download_target_path_edit_(new QLineEdit)
    , send_button_(new QPushButton(tr("Send File")))
    , download_button_(new QPushButton(tr("Download File")))
    , canvas_(new TestInfoCanvas)
    , bridge_(new TrainingClientBridge(this)) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    auto* upload_layout = new QHBoxLayout;
    auto* download_layout = new QHBoxLayout;
    auto* transfer_form = new QFormLayout;
    auto* choose_button = new QPushButton(tr("Select File"));
    auto* choose_download_button = new QPushButton(tr("Save As"));

    // Keep the top-level window simple: sync canvas first, then file actions.
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QFont title_font = title_label_->font();
    title_font.setPointSize(title_font.pointSize() + 4);
    title_font.setBold(true);
    title_label_->setFont(title_font);

    file_path_edit_->setReadOnly(true);
    file_path_edit_->setPlaceholderText(tr("No file selected"));
    upload_remote_path_edit_->setPlaceholderText(tr("Optional remote path, e.g. gui/demo.txt"));
    download_remote_path_edit_->setPlaceholderText(tr("Remote relative path, e.g. downloads/sample.bin"));
    download_target_path_edit_->setPlaceholderText(tr("Local target path"));
    send_button_->setEnabled(false);
    download_button_->setEnabled(true);
    file_transfer_label_->setStyleSheet(QStringLiteral("color: #666666;"));

    // 上传区保留原有“选文件再发送”的交互。
    upload_layout->addWidget(file_path_edit_, 1);
    upload_layout->addWidget(choose_button);
    upload_layout->addWidget(send_button_);

    // 下载区新增远端路径和本地保存路径输入。
    download_layout->addWidget(download_target_path_edit_, 1);
    download_layout->addWidget(choose_download_button);
    download_layout->addWidget(download_button_);

    transfer_form->addRow(tr("Upload file"), upload_layout);
    transfer_form->addRow(tr("Upload remote path"), upload_remote_path_edit_);
    transfer_form->addRow(tr("Download remote path"), download_remote_path_edit_);
    transfer_form->addRow(tr("Download target"), download_layout);

    layout->addWidget(title_label_);
    layout->addWidget(canvas_, 1);
    layout->addWidget(info_label_);
    layout->addLayout(transfer_form);
    layout->addWidget(file_transfer_label_);
    layout->addWidget(status_label_);

    setCentralWidget(central);
    setMinimumSize(560, 400);

    // The canvas emits user edits, and the bridge pushes them to the service.
    connect(canvas_, &TestInfoCanvas::checkStateEdited, bridge_, &TrainingClientBridge::submitCheckState);
    connect(canvas_, &TestInfoCanvas::textEditedByUser, bridge_, &TrainingClientBridge::submitText);
    connect(canvas_, &TestInfoCanvas::dragPreviewRequested, bridge_, &TrainingClientBridge::submitPreviewPosition);
    connect(canvas_, &TestInfoCanvas::dragCommitted, bridge_, &TrainingClientBridge::submitCommittedPosition);
    connect(bridge_, &TrainingClientBridge::infoChanged, this, &MainWindow::applyInfo);
    connect(bridge_, &TrainingClientBridge::backendError, this, &MainWindow::showBackendError);
    connect(bridge_, &TrainingClientBridge::operationStatusChanged, this, &MainWindow::updateOperationStatus);
    connect(bridge_, &TrainingClientBridge::fileSelectionChanged, this, &MainWindow::updateSelectedFile);
    connect(bridge_, &TrainingClientBridge::fileTransferResultChanged, this, &MainWindow::showFileTransferResult);
    connect(choose_button, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(choose_download_button, &QPushButton::clicked, this, &MainWindow::chooseDownloadTarget);
    connect(send_button_, &QPushButton::clicked, this, &MainWindow::sendSelectedFile);
    connect(download_button_, &QPushButton::clicked, this, &MainWindow::downloadFile);

    bridge_->start();
}

// Render the latest synchronized TestInfo in the status summary line.
void MainWindow::applyInfo(const training::public_api::TestInfo& info) {
    canvas_->applyInfo(info);
    info_label_->setText(
        tr("bool=%1, x=%2 + %3, text=\"%4\"")
            .arg(info.bool_param ? tr("true") : tr("false"))
            .arg(info.int_param)
            .arg(info.double_param, 0, 'f', 3)
            .arg(QString::fromStdString(info.string_param)));
}

// Backend errors stay separate from normal operation messages.
void MainWindow::showBackendError(const QString& message) {
    status_label_->setText(tr("Backend error: %1").arg(message));
}

// Normal actions such as connecting or selecting a file are shown here.
void MainWindow::updateOperationStatus(const QString& message) {
    status_label_->setText(message);
}

// Only enable sending when the user has actually picked a file.
void MainWindow::updateSelectedFile(const QString& path, bool has_selection) {
    if (has_selection) {
        file_path_edit_->setText(path);
        send_button_->setEnabled(true);
        return;
    }
    file_path_edit_->clear();
    send_button_->setEnabled(false);
}

// Use a dedicated result label so file-send success is not easy to miss.
void MainWindow::showFileTransferResult(bool success, const QString& message) {
    file_transfer_label_->setText(message);
    file_transfer_label_->setStyleSheet(success ? QStringLiteral("color: #1f7a1f;")
                                                : QStringLiteral("color: #b22222;"));
}

// QFileDialog is kept in the window so the bridge stays UI-agnostic.
void MainWindow::chooseFile() {
    const QString selected = QFileDialog::getOpenFileName(this, tr("Select File To Send"));
    bridge_->selectFilePath(selected);
}

void MainWindow::chooseDownloadTarget() {
    const QString selected = QFileDialog::getSaveFileName(this, tr("Select Download Target"));
    if (!selected.isEmpty()) {
        download_target_path_edit_->setText(selected);
    }
}

// The bridge performs the actual transport call.
void MainWindow::sendSelectedFile() {
    bridge_->sendSelectedFile(upload_remote_path_edit_->text());
}

// 下载调用桥接层，窗口只负责收集输入。
void MainWindow::downloadFile() {
    bridge_->downloadFile(download_remote_path_edit_->text(), download_target_path_edit_->text());
}

} // namespace syncdemo
