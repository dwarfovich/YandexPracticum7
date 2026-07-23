#include "headers.h"

#include <charconv>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;

void iterHeaders(std::string_view req, Callback &&callback) {
    auto pos = req.find_first_of("\r:\n");
    if (pos == std::string_view::npos) {
        return;
    }

    if (req[pos] == '\r') {
        if (req.size() > pos + 1 && req[pos + 1] == '\n') {
            pos += 2;
        } else {
            return;
        }
    } else if (req[pos] == ':') {
        pos = 0;
    }

    while (true) {
        if (pos >= req.size()) {
            return;
        }

        if (req.compare(pos, 2, "\r\n") == 0) {
            return;
        }

        auto colon = req.find(':', pos);
        if (colon == std::string_view::npos) {
            return;
        }

        auto lineEnd = req.find("\r\n", colon);
        if (lineEnd == std::string_view::npos) {
            lineEnd = req.size();
        }

        std::string_view header = req.substr(pos, colon - pos);
        std::size_t valueBegin = colon + 1;
        while (valueBegin < lineEnd && (req[valueBegin] == ' ' || req[valueBegin] == '\t')) {
            ++valueBegin;
        }

        std::string_view value = req.substr(valueBegin, lineEnd - valueBegin);
        callback(header, value);
        pos = lineEnd + 2;
    }
}

std::pair<std::string, std::string> findHostPort(std::string_view req) {
    static constexpr std::string_view hostHeaderText = "Host:";

    const auto pos = req.find(hostHeaderText);
    if (pos == std::string_view::npos)
        return {};

    auto hostStart = pos + hostHeaderText.size();

    while (hostStart < req.size() && req[hostStart] == ' ')
        ++hostStart;

    auto lineEnd = req.find("\r\n", hostStart);
    if (lineEnd == std::string_view::npos)
        return {};

    auto hostPort = req.substr(hostStart, lineEnd - hostStart);

    auto colon = hostPort.find(':');

    if (colon == std::string_view::npos) {
        return {std::string(hostPort), {}};
    }

    return {std::string(hostPort.substr(0, colon)), std::string(hostPort.substr(colon + 1))};
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    static const std::string contentLengthHeaderText = "Content-Length: ";
    const auto pos = rsp.find(contentLengthHeaderText);
    if (pos == std::string::npos) {
        return {};
    } else {
        const auto endPos = rsp.find("\r", pos + contentLengthHeaderText.size());
        if (endPos == std::string::npos) {
            return {};
        } else {
            std::string_view value{rsp.cbegin() + pos + contentLengthHeaderText.size(), rsp.cbegin() + endPos};
            std::size_t length = 0;
            std::from_chars(value.data(), value.data() + value.size(), length);
            return length;
        }
    }
}
