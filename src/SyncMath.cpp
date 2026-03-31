#include "SyncMath.hpp"

namespace syncdemo::math {

// Clamp all drag math into the legal canvas range.
double Clamp(double value, double minimum, double maximum) {
    return std::max(minimum, std::min(value, maximum));
}

// Preview keeps the fractional part so another window can follow smoothly.
CoordinateParts SplitPreviewCoordinate(double coordinate) {
    const double clamped = Clamp(coordinate, 0.0, MaxLogicalX());
    const int integer_part = static_cast<int>(std::floor(clamped));
    return CoordinateParts{integer_part, clamped - static_cast<double>(integer_part)};
}

// Commit snaps to an integer-aligned coordinate for the final sync value.
CoordinateParts SplitCommittedCoordinate(double coordinate) {
    const double clamped = Clamp(coordinate, 0.0, MaxLogicalX());
    return CoordinateParts{static_cast<int>(std::lround(clamped)), 0.0};
}

// Recompose the logical drag coordinate used by the canvas.
double ComposeCoordinate(const CoordinateParts& parts) {
    return static_cast<double>(parts.integer_part) + parts.fractional_part;
}

// Scale the canvas uniformly so relative geometry matches across windows.
double ScaleFactor(int current_width, int current_height) {
    const double scale_x = static_cast<double>(current_width) / static_cast<double>(kBaseCanvasWidth);
    const double scale_y = static_cast<double>(current_height) / static_cast<double>(kBaseCanvasHeight);
    return std::max(0.5, std::min(scale_x, scale_y));
}

// Prevent dragging beyond the visible work area.
double MaxLogicalX() {
    return static_cast<double>(kBaseCanvasWidth - (2 * kBaseMargin) - kBaseEditWidth);
}

// Convert the current pixel position back into the logical shared coordinate.
double PixelToLogicalX(double pixel_x, int current_width, int current_height) {
    const double scale = ScaleFactor(current_width, current_height);
    const double logical = (pixel_x / scale) - static_cast<double>(kBaseMargin);
    return Clamp(logical, 0.0, MaxLogicalX());
}

// Convert the shared logical coordinate into each window's local pixels.
double LogicalToPixelX(double logical_x, int current_width, int current_height) {
    const double scale = ScaleFactor(current_width, current_height);
    const double clamped = Clamp(logical_x, 0.0, MaxLogicalX());
    return static_cast<double>(kBaseMargin + clamped) * scale;
}

// Floating-point comparisons are only used for sync equivalence checks.
bool NearlyEqual(double left, double right) {
    return std::fabs(left - right) <= kCompareEpsilon;
}

} // namespace syncdemo::math
