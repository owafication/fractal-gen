#pragma once

#include "Core/StillImageRenderer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mw {

struct VisualFixtureDefinition {
    std::string id;
    std::string name;
    std::string coverage;
    std::string precisionExpectation;
    Preset preset;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t tiledWidth{0};
    double timeSeconds{0.0};
};

struct VisualImage {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Top-down BGRA pixels encoded as 0xAARRGGBB.
    std::vector<std::uint32_t> pixels;
};

struct VisualComparisonMetrics {
    std::uint64_t pixelCount{0};
    std::uint64_t differingPixels{0};
    std::uint8_t maximumChannelError{0};
    double differingPixelRatio{0.0};
    double meanAbsoluteChannelError{0.0};
    double rootMeanSquareChannelError{0.0};
    double structuralSimilarity{1.0};
    bool dimensionsMatch{false};
    bool exactMatch{false};
};

// Fixed, bounded fixtures for the canonical portable CPU production renderer.
[[nodiscard]] std::vector<VisualFixtureDefinition> BuiltInVisualFixtures();

// Renders through RenderStillImageTiled, collecting the complete bounded fixture image.
bool RenderVisualFixture(const VisualFixtureDefinition& fixture,
                         std::uint32_t tileWidth,
                         VisualImage& image,
                         StillRenderStatistics& statistics,
                         std::string& error);

[[nodiscard]] VisualComparisonMetrics CompareVisualImages(
    const VisualImage& expected,
    const VisualImage& actual,
    std::uint8_t errorFloor = 0U) noexcept;

[[nodiscard]] VisualImage CreateVisualDifferenceImage(
    const VisualImage& expected,
    const VisualImage& actual,
    std::uint8_t amplification = 4U);

// Fixture-local diagnostic hashes. These are deliberately not the canonical project
// render fingerprint planned for PH-03.
[[nodiscard]] std::uint64_t VisualImageFnv1a64(const VisualImage& image) noexcept;
[[nodiscard]] std::string VisualFixtureStateJson(const VisualFixtureDefinition& fixture);
[[nodiscard]] std::string HexDigest(std::uint64_t value);

} // namespace mw
