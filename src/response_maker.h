#pragma once

#include "http_server.h"

#include <filesystem>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;

    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;

    StringResponse MakeMethodNotAllowedResponse(http::status status,
                                        unsigned http_version,
                                        bool keep_alive,
                                        std::string_view content_type,
                                        std::string_view body,
                                        std::string_view relevant_method);

    StringResponse MakeStringResponse(http::status status,
                                      unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type,
                                      std::string_view body);

    FileResponse MakeFileResponse(http::status status,
                                  unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type,
                                  const std::filesystem::path& path);

}