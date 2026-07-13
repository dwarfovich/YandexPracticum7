#include "headers.h"

#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;

void iterHeaders(std::string_view req, Callback&& callback) {
    auto pos = req.find_first_of("\r:\n");
    if (pos == std::string_view::npos) {
        return;
    }

    if (req[pos] == '\r'){
        if (req.size() > pos && req[pos+1] == '\n'){
            pos += 2;
        } else {
            return;
        }
    } else if (req[pos] == ':'){
        pos = 0;
    }

    while (true) {
        if (pos >= req.size()){
            return;
        }

        if (req.compare(pos, 2, "\r\n") == 0){
            return;
        }

        auto colon = req.find(':', pos);
        if (colon == std::string_view::npos){
            return;
        }

        auto lineEnd = req.find("\r\n", colon);
        if (lineEnd == std::string_view::npos){
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
    return {};
  // code here
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    return {};
  // code here
}
