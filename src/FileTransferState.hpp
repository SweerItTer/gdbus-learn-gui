#pragma once

#include <string>

namespace syncdemo {

// Stores the current file selection so the window can enable/disable sending
// without mixing UI state into the transport bridge.
class FileTransferState {
public:
    void selectFile(std::string path);
    void clear();

    bool hasSelection() const;
    const std::string& displayPath() const;

private:
    std::string selected_path_;
};

} // namespace syncdemo
