#include "SyncMath.hpp"
#include "FileTransferState.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace syncdemo::math;
    using namespace syncdemo;

    const auto preview = SplitPreviewCoordinate(123.75);
    require(preview.integer_part == 123, "preview integer split");
    require(NearlyEqual(preview.fractional_part, 0.75), "preview fractional split");

    const auto committed = SplitCommittedCoordinate(123.75);
    require(committed.integer_part == 124, "committed rounds to nearest integer");
    require(NearlyEqual(committed.fractional_part, 0.0), "committed clears fraction");

    require(NearlyEqual(ComposeCoordinate({10, 0.25}), 10.25), "compose coordinate");

    const double logical = PixelToLogicalX(200.0, 800, 480);
    require(logical >= 0.0, "pixel to logical clamps lower bound");
    require(NearlyEqual(LogicalToPixelX(logical, 800, 480), 200.0), "logical to pixel inverse");

    require(NearlyEqual(ScaleFactor(400, 240), 0.5), "minimum scale limit");
    require(NearlyEqual(ScaleFactor(1600, 960), 2.0), "scale expands proportionally");

    FileTransferState transfer_state;
    require(!transfer_state.hasSelection(), "empty transfer state initially");
    require(transfer_state.displayPath().empty(), "display path initially empty");

    transfer_state.selectFile("/tmp/demo.bin");
    require(transfer_state.hasSelection(), "selection becomes available");
    require(transfer_state.displayPath() == "/tmp/demo.bin", "selected path is exposed");

    transfer_state.clear();
    require(!transfer_state.hasSelection(), "clear removes selection");
    require(transfer_state.displayPath().empty(), "clear empties display path");

    std::cout << "sync_math_tests: PASS\n";
    return 0;
}
