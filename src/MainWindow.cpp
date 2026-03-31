#include "MainWindow.hpp"

#include "TestInfoCanvas.hpp"

#include <QFileDialog>
#include <QFont>
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
    , status_label_(new QLabel(tr("Connecting...")))
    , file_path_edit_(new QLineEdit)
    , send_button_(new QPushButton(tr("Send File")))
    , canvas_(new TestInfoCanvas)
    , bridge_(new TrainingClientBridge(this)) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    auto* file_layout = new QHBoxLayout;
    auto* choose_button = new QPushButton(tr("Select File"));

    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QFont title_font = title_label_->font();
    title_font.setPointSize(title_font.pointSize() + 4);
    title_font.setBold(true);
    title_label_->setFont(title_font);

    file_path_edit_->setReadOnly(true);
    file_path_edit_->setPlaceholderText(tr("No file selected"));
    send_button_->setEnabled(false);
    file_transfer_label_->setStyleSheet(QStringLiteral("color: #666666;"));

    file_layout->addWidget(file_path_edit_, 1);
    file_layout->addWidget(choose_button);
    file_layout->addWidget(send_button_);

    layout->addWidget(title_label_);
    layout->addWidget(canvas_, 1);
    layout->addWidget(info_label_);
    layout->addLayout(file_layout);
    layout->addWidget(file_transfer_label_);
    layout->addWidget(status_label_);

    setCentralWidget(central);
    setMinimumSize(560, 400);

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
    connect(send_button_, &QPushButton::clicked, this, &MainWindow::sendSelectedFile);

    bridge_->start();
}

void MainWindow::applyInfo(const training::public_api::TestInfo& info) {
    canvas_->applyInfo(info);
    info_label_->setText(
        tr("bool=%1, x=%2 + %3, text=\"%4\"")
            .arg(info.bool_param ? tr("true") : tr("false"))
            .arg(info.int_param)
            .arg(info.double_param, 0, 'f', 3)
            .arg(QString::fromStdString(info.string_param)));
}

void MainWindow::showBackendError(const QString& message) {
    status_label_->setText(tr("Backend error: %1").arg(message));
}

void MainWindow::updateOperationStatus(const QString& message) {
    status_label_->setText(message);
}

void MainWindow::updateSelectedFile(const QString& path, bool has_selection) {
    if (has_selection) {
        file_path_edit_->setText(path);
        send_button_->setEnabled(true);
        return;
    }
    file_path_edit_->clear();
    send_button_->setEnabled(false);
}

void MainWindow::showFileTransferResult(bool success, const QString& message) {
    file_transfer_label_->setText(message);
    file_transfer_label_->setStyleSheet(success ? QStringLiteral("color: #1f7a1f;")
                                                : QStringLiteral("color: #b22222;"));
}

void MainWindow::chooseFile() {
    const QString selected = QFileDialog::getOpenFileName(this, tr("Select File To Send"));
    bridge_->selectFilePath(selected);
}

void MainWindow::sendSelectedFile() {
    bridge_->sendSelectedFile();
}

} // namespace syncdemo
