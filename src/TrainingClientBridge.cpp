#include "TrainingClientBridge.hpp"

#include "SyncMath.hpp"

#include <QMetaObject>

#include <exception>

namespace syncdemo {

TrainingClientBridge::TrainingClientBridge(QObject* parent)
    : QObject(parent) {}

training::public_api::TestInfo TrainingClientBridge::currentInfo() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_info_;
}

QString TrainingClientBridge::selectedFilePath() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return QString::fromStdString(file_transfer_state_.displayPath());
}

void TrainingClientBridge::start() {
    try {
        const auto info = GetTestInfo();
        updateCachedInfo(info);
        publishInfo(info);
        publishStatus(QStringLiteral("Connected to service"));
    } catch (const std::exception& ex) {
        publishError(QString::fromUtf8(ex.what()));
    }
}

void TrainingClientBridge::submitCheckState(bool checked) {
    auto info = buildUpdatedInfo();
    info.bool_param = checked;
    submitInfo(info);
}

void TrainingClientBridge::submitText(const QString& text) {
    auto info = buildUpdatedInfo();
    info.string_param = text.toStdString();
    submitInfo(info);
}

void TrainingClientBridge::submitPreviewPosition(double logical_x) {
    auto info = buildUpdatedInfo();
    const auto parts = math::SplitPreviewCoordinate(logical_x);
    info.int_param = parts.integer_part;
    info.double_param = parts.fractional_part;
    submitInfo(info);
}

void TrainingClientBridge::submitCommittedPosition(double logical_x) {
    auto info = buildUpdatedInfo();
    const auto parts = math::SplitCommittedCoordinate(logical_x);
    info.int_param = parts.integer_part;
    info.double_param = parts.fractional_part;
    submitInfo(info);
}

void TrainingClientBridge::selectFilePath(const QString& path) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (path.isEmpty()) {
            file_transfer_state_.clear();
        } else {
            file_transfer_state_.selectFile(path.toStdString());
        }
    }

    emit fileSelectionChanged(path, !path.isEmpty());
    if (!path.isEmpty()) {
        publishStatus(QStringLiteral("Selected file: %1").arg(path));
    }
}

void TrainingClientBridge::sendSelectedFile() {
    std::string selected_path;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!file_transfer_state_.hasSelection()) {
            publishError(QStringLiteral("No file selected"));
            return;
        }
        selected_path = file_transfer_state_.displayPath();
    }

    try {
        if (SendFileByPath(selected_path)) {
            QMetaObject::invokeMethod(
                this,
                [this, selected_path]() {
                    emit fileTransferResultChanged(
                        true, QStringLiteral("File sent successfully: %1").arg(QString::fromStdString(selected_path)));
                },
                Qt::QueuedConnection);
            publishStatus(QStringLiteral("File sent: %1").arg(QString::fromStdString(selected_path)));
            return;
        }
        QMetaObject::invokeMethod(
            this,
            [this]() { emit fileTransferResultChanged(false, QStringLiteral("File send reported failure")); },
            Qt::QueuedConnection);
        publishError(QStringLiteral("File send reported failure"));
    } catch (const std::exception& ex) {
        const QString message = QString::fromUtf8(ex.what());
        QMetaObject::invokeMethod(
            this,
            [this, message]() { emit fileTransferResultChanged(false, message); },
            Qt::QueuedConnection);
        publishError(QString::fromUtf8(ex.what()));
    }
}

void TrainingClientBridge::OnRemoteTestBoolChanged(bool param) {
    auto info = currentInfo();
    info.bool_param = param;
    updateCachedInfo(info);
    publishInfo(info);
}

void TrainingClientBridge::OnRemoteTestIntChanged(int param) {
    auto info = currentInfo();
    info.int_param = param;
    updateCachedInfo(info);
    publishInfo(info);
}

void TrainingClientBridge::OnRemoteTestDoubleChanged(double param) {
    auto info = currentInfo();
    info.double_param = param;
    updateCachedInfo(info);
    publishInfo(info);
}

void TrainingClientBridge::OnRemoteTestStringChanged(const std::string& param) {
    auto info = currentInfo();
    info.string_param = param;
    updateCachedInfo(info);
    publishInfo(info);
}

void TrainingClientBridge::OnRemoteTestInfoChanged(const training::public_api::TestInfo& param) {
    const auto previous = currentInfo();
    if (isSameInfo(previous, param)) {
        return;
    }

    updateCachedInfo(param);
    publishInfo(param);
}

training::public_api::TestInfo TrainingClientBridge::buildUpdatedInfo() const {
    return currentInfo();
}

void TrainingClientBridge::submitInfo(training::public_api::TestInfo info) {
    try {
        SetTestInfo(info);
        updateCachedInfo(info);
        publishInfo(info);
    } catch (const std::exception& ex) {
        publishError(QString::fromUtf8(ex.what()));
    }
}

void TrainingClientBridge::publishInfo(const training::public_api::TestInfo& info) {
    QMetaObject::invokeMethod(
        this,
        [this, info]() { emit infoChanged(info); },
        Qt::QueuedConnection);
}

void TrainingClientBridge::publishStatus(const QString& message) {
    QMetaObject::invokeMethod(
        this,
        [this, message]() { emit operationStatusChanged(message); },
        Qt::QueuedConnection);
}

void TrainingClientBridge::publishError(const QString& message) {
    QMetaObject::invokeMethod(
        this,
        [this, message]() { emit backendError(message); },
        Qt::QueuedConnection);
}

bool TrainingClientBridge::isSameInfo(const training::public_api::TestInfo& left,
                                      const training::public_api::TestInfo& right) const {
    return left.bool_param == right.bool_param &&
           left.int_param == right.int_param &&
           math::NearlyEqual(left.double_param, right.double_param) &&
           left.string_param == right.string_param;
}

void TrainingClientBridge::updateCachedInfo(const training::public_api::TestInfo& info) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_info_ = info;
}

} // namespace syncdemo
