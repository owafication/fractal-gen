#include "Core/Precision/ExactDecimal.h"

#include <charconv>
#include <cmath>
#include <limits>

namespace mw {
namespace {

bool IsDigit(char value) noexcept {
    return value >= '0' && value <= '9';
}

bool ParseExponent(std::string_view text, std::size_t& offset, int& exponent,
                   std::string& error) {
    if (offset == text.size()) return true;
    if (text[offset] != 'e' && text[offset] != 'E') {
        error = "Exact decimal contains an unsupported character.";
        return false;
    }
    ++offset;
    bool negative = false;
    if (offset < text.size() && (text[offset] == '+' || text[offset] == '-')) {
        negative = text[offset] == '-';
        ++offset;
    }
    const std::size_t digitsBegin = offset;
    const int parseLimit = ExactDecimal::kMaximumExponentMagnitude +
                           static_cast<int>(ExactDecimal::kMaximumSignificantDigits);
    int magnitude = 0;
    while (offset < text.size() && IsDigit(text[offset])) {
        const int digit = text[offset] - '0';
        if (magnitude > (parseLimit - digit) / 10) {
            error = "Exact decimal exponent exceeds the configured bound.";
            return false;
        }
        magnitude = magnitude * 10 + digit;
        ++offset;
    }
    if (offset != text.size() || offset == digitsBegin) {
        error = "Exact decimal exponent is malformed.";
        return false;
    }
    exponent = negative ? -magnitude : magnitude;
    return true;
}

} // namespace

bool ExactDecimal::Parse(std::string_view text, ExactDecimal& result, std::string& error) {
    result = {};
    error.clear();
    if (text.empty() || text.size() > kMaximumInputBytes) {
        error = "Exact decimal is empty or exceeds the 16 KiB input bound.";
        return false;
    }

    std::size_t offset = 0U;
    bool negative = false;
    if (text[offset] == '+' || text[offset] == '-') {
        negative = text[offset] == '-';
        if (++offset == text.size()) {
            error = "Exact decimal is missing digits.";
            return false;
        }
    }

    std::string digits;
    digits.reserve(text.size());
    std::size_t digitsBeforePoint = 0U;
    bool sawPoint = false;
    while (offset < text.size()) {
        const char current = text[offset];
        if (IsDigit(current)) {
            digits.push_back(current);
            if (!sawPoint) ++digitsBeforePoint;
            ++offset;
            continue;
        }
        if (current == '.' && !sawPoint) {
            sawPoint = true;
            ++offset;
            continue;
        }
        break;
    }
    if (digits.empty()) {
        error = "Exact decimal is missing digits.";
        return false;
    }

    int suppliedExponent = 0;
    if (!ParseExponent(text, offset, suppliedExponent, error)) return false;

    const std::size_t first = digits.find_first_not_of('0');
    if (first == std::string::npos) {
        result.canonicalText_ = "0";
        return true;
    }
    const std::size_t last = digits.find_last_not_of('0');
    const std::size_t significantCount = last - first + 1U;
    if (significantCount > kMaximumSignificantDigits) {
        error = "Exact decimal exceeds the 8,192 significant-digit bound.";
        return false;
    }

    const long long normalizedExponent = static_cast<long long>(suppliedExponent) +
        static_cast<long long>(digitsBeforePoint) - static_cast<long long>(first) - 1LL;
    if (normalizedExponent < -kMaximumExponentMagnitude ||
        normalizedExponent > kMaximumExponentMagnitude) {
        error = "Exact decimal normalized exponent exceeds the configured bound.";
        return false;
    }

    std::string canonical;
    canonical.reserve(significantCount + 16U);
    if (negative) canonical.push_back('-');
    canonical.push_back(digits[first]);
    if (significantCount > 1U) {
        canonical.push_back('.');
        canonical.append(digits, first + 1U, significantCount - 1U);
    }
    canonical.push_back('e');
    canonical.append(std::to_string(normalizedExponent));
    result.canonicalText_ = std::move(canonical);
    return true;
}

bool ExactDecimal::FromFiniteDouble(double value, ExactDecimal& result, std::string& error) {
    if (!std::isfinite(value)) {
        result = {};
        error = "An exact decimal cannot be constructed from a non-finite double.";
        return false;
    }
    char buffer[std::numeric_limits<double>::max_digits10 + 16U]{};
    const auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value,
                                         std::chars_format::scientific,
                                         std::numeric_limits<double>::max_digits10);
    if (converted.ec != std::errc{}) {
        result = {};
        error = "Could not format finite double as an exact decimal.";
        return false;
    }
    return Parse(std::string_view(buffer, static_cast<std::size_t>(converted.ptr - buffer)),
                 result, error);
}

bool ExactDecimal::Add(const ExactDecimal& left, const ExactDecimal& right,
                       ExactDecimal& result, std::string& error) {
    result = {};
    error.clear();
    if (left.IsZero()) { result = right; return true; }
    if (right.IsZero()) { result = left; return true; }

    struct Parts {
        bool negative{false};
        std::string digits;
        int lowPower{0};
    };
    const auto split = [](const ExactDecimal& value, Parts& parts) {
        std::string_view text = value.CanonicalText();
        parts.negative = text.front() == '-';
        if (parts.negative) text.remove_prefix(1U);
        const std::size_t exponentMarker = text.find('e');
        const std::string_view significand = text.substr(0U, exponentMarker);
        for (const char character : significand) {
            if (character != '.') parts.digits.push_back(character);
        }
        int exponent = 0;
        (void)std::from_chars(text.data() + exponentMarker + 1U, text.data() + text.size(), exponent);
        parts.lowPower = exponent - static_cast<int>(parts.digits.size()) + 1;
    };
    Parts a;
    Parts b;
    split(left, a);
    split(right, b);
    const int commonPower = std::min(a.lowPower, b.lowPower);
    const std::size_t aZeroes = static_cast<std::size_t>(a.lowPower - commonPower);
    const std::size_t bZeroes = static_cast<std::size_t>(b.lowPower - commonPower);
    if (a.digits.size() + aZeroes > kMaximumSignificantDigits ||
        b.digits.size() + bZeroes > kMaximumSignificantDigits) {
        error = "Exact decimal addition would exceed the significant-digit bound.";
        return false;
    }
    a.digits.append(aZeroes, '0');
    b.digits.append(bZeroes, '0');
    const std::size_t width = std::max(a.digits.size(), b.digits.size());
    a.digits.insert(0U, width - a.digits.size(), '0');
    b.digits.insert(0U, width - b.digits.size(), '0');

    std::string magnitude(width, '0');
    bool negative = false;
    if (a.negative == b.negative) {
        int carry = 0;
        for (std::size_t index = width; index-- > 0U;) {
            const int sum = (a.digits[index] - '0') + (b.digits[index] - '0') + carry;
            magnitude[index] = static_cast<char>('0' + (sum % 10));
            carry = sum / 10;
        }
        if (carry != 0) magnitude.insert(magnitude.begin(), static_cast<char>('0' + carry));
        negative = a.negative;
    } else {
        const bool aLarger = a.digits >= b.digits;
        const std::string& larger = aLarger ? a.digits : b.digits;
        const std::string& smaller = aLarger ? b.digits : a.digits;
        int borrow = 0;
        for (std::size_t index = width; index-- > 0U;) {
            int difference = (larger[index] - '0') - (smaller[index] - '0') - borrow;
            if (difference < 0) { difference += 10; borrow = 1; } else { borrow = 0; }
            magnitude[index] = static_cast<char>('0' + difference);
        }
        negative = aLarger ? a.negative : b.negative;
    }
    const std::size_t first = magnitude.find_first_not_of('0');
    if (first == std::string::npos) { result = {}; return true; }
    const std::size_t digits = magnitude.size() - first;
    if (digits > kMaximumSignificantDigits) {
        error = "Exact decimal addition result exceeds the significant-digit bound.";
        return false;
    }
    const long long exponent = static_cast<long long>(commonPower) +
        static_cast<long long>(magnitude.size()) - 1LL;
    if (exponent < -kMaximumExponentMagnitude || exponent > kMaximumExponentMagnitude) {
        error = "Exact decimal addition result exceeds the exponent bound.";
        return false;
    }
    std::string canonical;
    if (negative) canonical.push_back('-');
    canonical.push_back(magnitude[first]);
    if (digits > 1U) {
        canonical.push_back('.');
        canonical.append(magnitude, first + 1U, digits - 1U);
    }
    canonical.push_back('e');
    canonical.append(std::to_string(exponent));
    return Parse(canonical, result, error);
}

} // namespace mw
