#include "request_handler.h"

namespace http_handler {

    std::string RequestHandler::ParseURL(std::string_view base_url) {
        std::string ans;
        ans.reserve(base_url.size());

        for (int i = 0; i < base_url.size(); ++i) {
            switch (base_url[i]) {
                case '+':
                    ans.push_back(' ');
                    break;
                case '%':
                    ans.push_back(std::stoi(std::string(base_url.substr(i + 1, 2)), 0, 16));
                    i += 2;
                    break;
                default:
                    ans.push_back(base_url[i]);
            }
        }

        return ans;
    }
}  // namespace http_handler