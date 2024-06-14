#pragma once

#include "http_handler_string_constants.h"
#include "response_maker.h"

#include <unordered_map>
#include <string_view>
#include <filesystem>
#include <variant>

#include <iostream>

namespace http_handler {
    using Response = std::variant<StringResponse, FileResponse>;

    class StaticRequestParser {
    public:
        StaticRequestParser() = delete;
        explicit StaticRequestParser(std::filesystem::path&& static_path)
                : static_path_(std::move(static_path)) {

        }

        StaticRequestParser(const StaticRequestParser&) = delete;
        StaticRequestParser& operator=(const StaticRequestParser&) = delete;

        template <typename Body, typename Allocator>
        Response ParseFileRequest(http::request<Body, http::basic_fields<Allocator>>&& req,
                                  std::string_view query) const {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                return MakeStringResponse(http::status::method_not_allowed,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::TEXT_HTML,
                                          ErrorMessages::invalidMethod);
            }

            std::filesystem::path abs_path = GetFilePath(query);

            if (!IsSubPath(abs_path, static_path_)) {
                return MakeStringResponse(http::status::bad_request,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::TEXT_PLAIN,
                                          ErrorMessages::badRequest);
            }

            std::string ext = abs_path.extension().string();

            if (!fs::exists(abs_path)) {
                return MakeStringResponse(http::status::not_found,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::TEXT_PLAIN,
                                          ErrorMessages::fileNotFound);
            }

            std::string_view content_type = ((file_format_to_content_type_.count(ext)) ?
                                             file_format_to_content_type_.at(ext) : ContentType::APPLICATION_OCTET_STREAM);

            return MakeFileResponse(http::status::ok,
                                    req.version(),
                                    req.keep_alive(),
                                    content_type,
                                    abs_path);
        }

    private:
        const std::filesystem::path static_path_;

        //Структура, которая преобразует расширение файла в содержимое заголовка ContentType
        using FFToCT = std::unordered_map<std::string_view, std::string_view>;
        const FFToCT file_format_to_content_type_{{FileFormats::html, ContentType::TEXT_HTML},
                                                  {FileFormats::js, ContentType::TEXT_JAVASCRIPT},
                                                  {FileFormats::json, ContentType::APPLICATION_JSON},
                                                  {FileFormats::svg, ContentType::IMAGE_SVG},
                                                  {FileFormats::png, ContentType::IMAGE_PNG}};

        std::filesystem::path GetFilePath(std::string_view query) const;
        // Возвращает true, если каталог p содержится внутри base_path.
        static bool IsSubPath(fs::path path, fs::path base);
    };
}
