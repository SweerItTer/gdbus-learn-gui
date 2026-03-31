#include "SyncMath.hpp"

namespace syncdemo::math {

double Clamp(double value, double minimum, double maximum) {
    return std::max(minimum, std::min(value, maximum));
}

CoordinateParts SplitPreviewCoordinate(double coordinate) {
    const double clamped = Clamp(coordinate, 0.0, MaxLogicalX());
    const int integer_part = static_cast<int>(std::floor(clamped));
    return CoordinateParts{integer_part, clamped - static_cast<double>(integer_part)};
}

CoordinateParts SplitCommittedCoordinate(double coordinate) {
    const double clamped = Clamp(coordinate, 0.0, MaxLogicalX());
    return CoordinateParts{static_cast<int>(std::lround(clamped)), 0.0};
}

double ComposeCoordinate(const CoordinateParts& parts) {
    return static_cast<double>(parts.integer_part) + parts.fractional_part;
}

double ScaleFactor(int current_width, int current_height) {
    const double scale_x = static_cast<double>(current_width) / static_cast<double>(kBaseCanvasWidth);
    const double scale_y = static_cast<double>(current_height) / static_cast<double>(kBaseCanvasHeight);
    return std::max(0.5, std::min(scale_x, scale_y));
}

double MaxLogicalX() {
    return static_cast<double>(kBaseCanvasWidth - (2 * kBaseMargin) - kBaseEditWidth);
}

double PixelToLogicalX(double pixel_x, int current_width, int current_height) {
    const double scale = ScaleFactor(current_width, current_height);
    const double logical = (pixel_x / scale) - static_cast<double>(kBaseMargin);
    return Clamp(logical, 0.0, MaxLogicalX());
}

double LogicalToPixelX(double logical_x, int current_width, int current_height) {
    const double scale = ScaleFactor(current_width, current_height);
    const double clamped = Clamp(logical_x, 0.0, MaxLogicalX());
    return static_cast<double>(kBaseMargin + clamped) * scale;
}

bool NearlyEqual(double left, double right) {
    return std::fabs(left - right) <= kCompareEpsilon;
}

} // namespace syncdemo::math
