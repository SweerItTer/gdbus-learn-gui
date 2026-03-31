#include "TrainingClientBridge.hpp"

#include "SyncMath.hpp"

#include <QMetaObject>

#include <exception>

namespace syncdemo {

TrainingClientBridge::TrainingClientBridge(QObject* parent)
    : QObject(parent) {}

// Return a copy so callers cannot race with remote callback updates.
training::public_api::TestInfo TrainingClientBridge::currentInfo() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_info_;
}

// The selected path is only GUI state and is not part of TestInfo.
QString TrainingClientBridge::selectedFilePath() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return QString::fromStdString(file_transfer_state_.displayPath());
}

// Pull the initial snapshot once; later changes come from inherited callbacks.
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

// Local UI changes are folded back into a complete TestInfo and sent upstream.
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

// Preview updates retain fractional precision while the user is dragging.
void TrainingClientBridge::submitPreviewPosition(double logical_x) {
    auto info = buildUpdatedInfo();
    const auto parts = math::SplitPreviewCoordinate(logical_x);
    info.int_param = parts.integer_part;
    info.double_param = parts.fractional_part;
    submitInfo(info);
}

// Commit updates snap to the final integer-aligned coordinate.
void TrainingClientBridge::submitCommittedPosition(double logical_x) {
    auto info = buildUpdatedInfo();
    const auto parts = math::SplitCommittedCoordinate(logical_x);
    info.int_param = parts.integer_part;
    info.double_param = parts.fractional_part;
    submitInfo(info);
}

// Store the file path locally and notify the window so it can update buttons.
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

// File transfer reuses the upstream SendFileByPath() API directly.
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

// Each remote callback updates just the changed field, then emits a full
// snapshot so both windows stay consistent.
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

// Full TestInfo broadcasts are the simplest path when the backend already
// emits a complete synchronized state.
void TrainingClientBridge::OnRemoteTestInfoChanged(const training::public_api::TestInfo& param) {
    const auto previous = currentInfo();
    if (isSameInfo(previous, param)) {
        return;
    }

    updateCachedInfo(param);
    publishInfo(param);
}

// Build the outgoing payload from the latest cached shared state.
training::public_api::TestInfo TrainingClientBridge::buildUpdatedInfo() const {
    return currentInfo();
}

// All UI-originated sync updates currently go through SetTestInfo().
void TrainingClientBridge::submitInfo(training::public_api::TestInfo info) {
    try {
        SetTestInfo(info);
        updateCachedInfo(info);
        publishInfo(info);
    } catch (const std::exception& ex) {
        publishError(QString::fromUtf8(ex.what()));
    }
}

// Signal emission is queued so remote callbacks never touch widgets directly.
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

// Equality checks suppress redundant UI churn when the same state is replayed.
bool TrainingClientBridge::isSameInfo(const training::public_api::TestInfo& left,
                                      const training::public_api::TestInfo& right) const {
    return left.bool_param == right.bool_param &&
           left.int_param == right.int_param &&
           math::NearlyEqual(left.double_param, right.double_param) &&
           left.string_param == right.string_param;
}

// Keep the cached snapshot isolated behind the mutex.
void TrainingClientBridge::updateCachedInfo(const training::public_api::TestInfo& info) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_info_ = info;
}

} // namespace syncdemo
