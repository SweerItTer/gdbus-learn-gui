#pragma once

#include "FileTransferState.hpp"

#include <client/TrainingClient.hpp>
#include <public/DbusConstants.hpp>

#include <QObject>
#include <QString>

#include <mutex>

namespace syncdemo {

// TrainingClientBridge adapts the inheritable gdbus-learn client into Qt
// signals so the window can stay mostly presentation-focused.
class TrainingClientBridge final : public QObject, public training::client::TrainingClient {
    Q_OBJECT

public:
    explicit TrainingClientBridge(QObject* parent = nullptr);

    training::public_api::TestInfo currentInfo() const;
    QString selectedFilePath() const;

public slots:
    // Slots are used for direct signal wiring from the Qt widgets.
    void start();
    void submitCheckState(bool checked);
    void submitText(const QString& text);
    void submitPreviewPosition(double logical_x);
    void submitCommittedPosition(double logical_x);
    void selectFilePath(const QString& path);
    // 上传继续复用当前选中文件，远端相对路径允许为空。
    void sendSelectedFile(const QString& remote_relative_path);
    // 下载需要同时给出远端相对路径和本地目标路径。
    void downloadFile(const QString& remote_relative_path, const QString& local_target_path);

signals:
    // infoChanged mirrors TestInfo synchronization, while the others keep the
    // file-send path and status messages explicit in the UI.
    void infoChanged(const training::public_api::TestInfo& info);
    void fileSelectionChanged(const QString& path, bool has_selection);
    void fileTransferResultChanged(bool success, const QString& message);
    void operationStatusChanged(const QString& message);
    void backendError(const QString& message);

protected:
    // The upstream client is inheritable, so remote callbacks are translated
    // here instead of being handled by a polling timer.
    void OnRemoteTestBoolChanged(bool param) override;
    void OnRemoteTestIntChanged(int param) override;
    void OnRemoteTestDoubleChanged(double param) override;
    void OnRemoteTestStringChanged(const std::string& param) override;
    void OnRemoteTestInfoChanged(const training::public_api::TestInfo& param) override;

private:
    // The bridge keeps a thread-safe copy of the latest TestInfo because
    // gdbus callbacks can arrive outside the Qt UI thread.
    training::public_api::TestInfo buildUpdatedInfo() const;
    void submitInfo(training::public_api::TestInfo info);
    void publishInfo(const training::public_api::TestInfo& info);
    void publishStatus(const QString& message);
    void publishError(const QString& message);
    bool isSameInfo(const training::public_api::TestInfo& left,
                    const training::public_api::TestInfo& right) const;
    void updateCachedInfo(const training::public_api::TestInfo& info);

    mutable std::mutex state_mutex_;
    training::public_api::TestInfo current_info_{};
    FileTransferState file_transfer_state_;
};

} // namespace syncdemo
