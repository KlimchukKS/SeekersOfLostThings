#include "response_maker.h"

namespace http_handler {

    StringResponse MakeMethodNotAllowedResponse(http::status status,
                                                unsigned http_version,
                                                bool keep_alive,
                                                std::string_view content_type,
                                                std::string_view body,
                                                std::string_view relevant_method) {
        constexpr static std::string_view no_cache{"no-cache"};
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        response.set(http::field::cache_control, no_cache);
        response.set(http::field::allow, relevant_method);
        return response;
    }

    StringResponse MakeStringResponse(http::status status,
                                      unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type,
                                      std::string_view body) {
        constexpr static std::string_view no_cache{"no-cache"};
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        response.set(http::field::cache_control, no_cache);
        return response;
    }

    FileResponse MakeFileResponse(http::status status,
                                  unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type,
                                  const std::filesystem::path& path) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);

        http::file_body::value_type file;

        if (boost::system::error_code ec; file.open(path.string().c_str(), beast::file_mode::read, ec), ec) {
            throw std::runtime_error("Failed to open file " + path.string());
        }

        response.body() = std::move(file);
        // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
        // в зависимости от свойств тела сообщения
        response.prepare_payload();
        response.keep_alive(keep_alive);

        return response;
    }

}
