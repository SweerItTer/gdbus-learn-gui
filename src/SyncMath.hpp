#pragma once

#include <algorithm>
#include <cmath>

namespace syncdemo::math {

// CoordinateParts mirrors the way TestInfo stores drag positions:
// integer cells plus a fractional preview part.
struct CoordinateParts {
    int integer_part{};
    double fractional_part{};
};

constexpr int kBaseCanvasWidth = 800;
constexpr int kBaseCanvasHeight = 480;
constexpr int kBaseEditWidth = 220;
constexpr int kBaseEditHeight = 36;
constexpr int kBaseMargin = 40;
constexpr int kBaseTop = 120;
constexpr double kCompareEpsilon = 1e-6;

// Helper functions keep the drag and scaling rules testable outside Qt.
double Clamp(double value, double minimum, double maximum);
CoordinateParts SplitPreviewCoordinate(double coordinate);
CoordinateParts SplitCommittedCoordinate(double coordinate);
double ComposeCoordinate(const CoordinateParts& parts);
double ScaleFactor(int current_width, int current_height);
double MaxLogicalX();
double PixelToLogicalX(double pixel_x, int current_width, int current_height);
double LogicalToPixelX(double logical_x, int current_width, int current_height);
bool NearlyEqual(double left, double right);

} // namespace syncdemo::math
