#include "headers.h"

#include <charconv>
#include <ranges>
#include <string_view>
#include <algorithm>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;

std::string_view trim(std::string_view str) {
    auto first = std::ranges::find_if_not(str, isspace);
    auto last = std::ranges::find_if_not(std::views::reverse(str), isspace).base();

    return std::string_view(first, last);
}

void iterHeaders(std::string_view req, Callback &&callback) {
    static constexpr std::size_t maximumHeaderLines = 100;
    for (auto line : std::views::split(req, std::string_view{"\r\n"}) | std::views::take(maximumHeaderLines)) {
        auto colon = std::ranges::find(line, ':');

        if (colon == line.end()) {
            continue;
        }

        auto header = std::string { line.begin(), colon };
        std::transform(header.begin(), header.end(), header.begin(), ::tolower);
        auto headerView = trim(header);
        auto valueView = trim(std::string_view{std::next(colon), line.end()});

        callback(headerView, valueView);
    }
}

std::pair<std::string, std::string> findHostPort(std::string_view req) {
    static constexpr std::string_view hostHeaderText = "host";

    std::string_view addressValue;

    iterHeaders(req, [&](const auto &header, const auto &value) {
        if (header == hostHeaderText) {
            addressValue = value;
        }
    });

    auto colon = addressValue.find(':');
    if (colon == std::string_view::npos) {
        return {std::string(addressValue), {}};
    }

    auto portView = addressValue.substr(colon + 1);
    std::size_t port;
    auto [ptr, ec] = std::from_chars(portView.data(), portView.data() + portView.size(), port);
    if (ec != std::errc{} || ptr != portView.data() + portView.size() || port > maxPort) {
        return {std::string(addressValue), {}};
    }

    return {std::string(addressValue.substr(0, colon)), std::string{portView}};
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    static constexpr std::string_view contentLengthHeaderText = "content-length";

    std::string_view lengthStr;
    iterHeaders(rsp, [&](const auto &header, const auto &value) {
        if (header == contentLengthHeaderText) {
            lengthStr = value;
        }
    });

    if (lengthStr.empty()) {
        return std::nullopt;
    }

    std::size_t value{};
    auto [ptr, ec] = std::from_chars(lengthStr.data(), lengthStr.data() + lengthStr.size(), value);
    if (ec != std::errc{} || ptr != lengthStr.data() + lengthStr.size()) {
        return std::nullopt;
    }

    return value;
}
