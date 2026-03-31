#pragma once

#include "FileTransferState.hpp"

#include <client/TrainingClient.hpp>
#include <public/DbusConstants.hpp>

#include <QObject>
#include <QString>

#include <mutex>

namespace syncdemo {

class TrainingClientBridge final : public QObject, public training::client::TrainingClient {
    Q_OBJECT

public:
    explicit TrainingClientBridge(QObject* parent = nullptr);

    training::public_api::TestInfo currentInfo() const;
    QString selectedFilePath() const;

public slots:
    void start();
    void submitCheckState(bool checked);
    void submitText(const QString& text);
    void submitPreviewPosition(double logical_x);
    void submitCommittedPosition(double logical_x);
    void selectFilePath(const QString& path);
    void sendSelectedFile();

signals:
    void infoChanged(const training::public_api::TestInfo& info);
    void fileSelectionChanged(const QString& path, bool has_selection);
    void fileTransferResultChanged(bool success, const QString& message);
    void operationStatusChanged(const QString& message);
    void backendError(const QString& message);

protected:
    void OnRemoteTestBoolChanged(bool param) override;
    void OnRemoteTestIntChanged(int param) override;
    void OnRemoteTestDoubleChanged(double param) override;
    void OnRemoteTestStringChanged(const std::string& param) override;
    void OnRemoteTestInfoChanged(const training::public_api::TestInfo& param) override;

private:
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
