#pragma once

#include <string_view>

namespace http_handler {
    using namespace std::string_view_literals;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
        constexpr static std::string_view TEXT_JAVASCRIPT = "text/javascript"sv;
        constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;
        constexpr static std::string_view IMAGE_PNG = "image/png"sv;
        constexpr static std::string_view APPLICATION_OCTET_STREAM = "application/octet-stream"sv;
    };

    struct ErrorMessages {
        ErrorMessages() = delete;
        constexpr static std::string_view mapNotFound = "{\n"
                                                        "  \"code\": \"mapNotFound\",\n"
                                                        "  \"message\": \"Map not found\"\n"
                                                        "}"sv;
        constexpr static std::string_view badRequest = "{\n"
                                                       "  \"code\": \"badRequest\",\n"
                                                       "  \"message\": \"Bad request\"\n"
                                                       "}"sv;
        constexpr static std::string_view fileNotFound = "file not found"sv;
        constexpr static std::string_view invalidMethod = "{\"code\": \"invalidMethod\", \"message\": \"Invalid method\"}"sv;
        constexpr static std::string_view invalidArgumentApiJoinJson = "{\"code\": \"invalidArgument\", \"message\": \"Join game request parse error\"}"sv;
        constexpr static std::string_view invalidMethodApiJoin = "{\"code\": \"invalidMethod\", \"message\": \"Only POST method is expected\"}"sv;
        constexpr static std::string_view invalidArgumentApiJoinName = "{\"code\": \"invalidArgument\", \"message\": \"Invalid name\"}"sv;
        constexpr static std::string_view invalidToken = "{\"code\": \"invalidToken\", \"message\": \"Authorization header is missing\"}"sv;
        constexpr static std::string_view unknownToken = "{\"code\": \"unknownToken\", \"message\": \"Player token has not been found\"}"sv;
        constexpr static std::string_view invalidArgumentToParseAction = "{\"code\": \"invalidArgument\", \"message\": \"Failed to parse action\"}"sv;
        constexpr static std::string_view invalidArgumentToParseJSON = "{\"code\": \"invalidArgument\", \"message\": \"Failed to parse tick request JSON\"}"sv;
    };

    struct ApiRequestType {
        constexpr static std::string_view api = "/api/"sv;
        constexpr static std::string_view maps = "/api/v1/maps"sv;
        constexpr static std::string_view join = "/api/v1/game/join"sv;
        constexpr static std::string_view players = "/api/v1/game/players"sv;
        constexpr static std::string_view state = "/api/v1/game/state"sv;
        constexpr static std::string_view action = "/api/v1/game/player/action"sv;
        constexpr static std::string_view tick = "/api/v1/game/tick"sv;
    };

    struct GetFileRequestType {
        constexpr static std::string_view index = "index.html"sv;
    };

    struct FileFormats {
        constexpr static std::string_view html = ".html"sv;
        constexpr static std::string_view js = ".js"sv;
        constexpr static std::string_view json = ".json"sv;
        constexpr static std::string_view svg = ".svg"sv;
        constexpr static std::string_view png = ".png"sv;
    };
}