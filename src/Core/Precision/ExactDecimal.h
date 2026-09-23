#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace mw {

// A bounded canonical decimal value. It deliberately stores text, never a
// floating-point approximation, so persistence and identity can retain the
// exact user/migrated value until a renderer explicitly requests an adapter.
class ExactDecimal final {
public:
    static constexpr std::size_t kMaximumInputBytes = 16U * 1024U;
    static constexpr std::size_t kMaximumSignificantDigits = 8192U;
    static constexpr int kMaximumExponentMagnitude = 1000000;

    [[nodiscard]] static bool Parse(std::string_view text, ExactDecimal& result,
                                    std::string& error);
    [[nodiscard]] static bool FromFiniteDouble(double value, ExactDecimal& result,
                                               std::string& error);
    [[nodiscard]] static bool Add(const ExactDecimal& left, const ExactDecimal& right,
                                  ExactDecimal& result, std::string& error);

    [[nodiscard]] const std::string& CanonicalText() const noexcept { return canonicalText_; }
    [[nodiscard]] bool IsZero() const noexcept { return canonicalText_ == "0"; }

    bool operator==(const ExactDecimal&) const = default;

private:
    std::string canonicalText_{"0"};
};

} // namespace mw
