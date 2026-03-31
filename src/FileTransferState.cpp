#include "FileTransferState.hpp"

namespace syncdemo {

void FileTransferState::selectFile(std::string path) {
    selected_path_ = std::move(path);
}

void FileTransferState::clear() {
    selected_path_.clear();
}

bool FileTransferState::hasSelection() const {
    return !selected_path_.empty();
}

const std::string& FileTransferState::displayPath() const {
    return selected_path_;
}

} // namespace syncdemo
