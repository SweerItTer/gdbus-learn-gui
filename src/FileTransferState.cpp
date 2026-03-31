#include "FileTransferState.hpp"

namespace syncdemo {

// Replace the previous selection with the latest path chosen by the user.
void FileTransferState::selectFile(std::string path) {
    selected_path_ = std::move(path);
}

// Clear the selection after the UI or user resets the file picker state.
void FileTransferState::clear() {
    selected_path_.clear();
}

// The send action is only valid when a path has been selected.
bool FileTransferState::hasSelection() const {
    return !selected_path_.empty();
}

// Expose the path for display and for SendFileByPath().
const std::string& FileTransferState::displayPath() const {
    return selected_path_;
}

} // namespace syncdemo
