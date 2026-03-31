#pragma once

#include <string>

namespace syncdemo {

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
