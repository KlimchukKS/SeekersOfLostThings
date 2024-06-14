#pragma once

#include <boost/json.hpp>
#include <boost/beast/http.hpp>

#include <variant>
#include <string_view>

namespace server_logging {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    using namespace std::string_view_literals;

    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;
    using Response = std::variant<StringResponse, FileResponse>;
    using Logger = std::function<void(json::value, std::string_view)>;

    class LoggingRequestHandler {
        template <typename Body, typename Allocator>
        static void LogRequest(const Logger& log, const http::request<Body, http::basic_fields<Allocator>>& req, std::string&& user_ip_endpoint) {
            json::value request_received_log{{"ip", user_ip_endpoint.data()},
                                             {"URI", req.target()},
                                             {"method", req.method_string()}};

            log(std::move(request_received_log), "request received"sv);
        }

        template<class Resp>
        static void LogResponse(const Logger& log, const Resp& response, int total_time_parse_res) {
            json::value response_sent_log{{"response_time", total_time_parse_res},
                                          {"code", response.result_int()},
                                          {"content_type", response[http::field::content_type]}};

            log(std::move(response_sent_log), "response sent"sv);
        }

    public:
        explicit LoggingRequestHandler(std::shared_ptr<http_handler::RequestHandler> decorated)
                : decorated_(decorated) {

        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                        Send&& send,
                        std::string&& user_ip_endpoint,
                        const Logger& log) {
            LogRequest(log, req, std::move(user_ip_endpoint));

            std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();

            Response response = (*decorated_)(std::forward<decltype(req)>(req));

            std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();

            auto total_time = (end_ts - start_ts_).count();
            auto send_response_and_log = [total_time, send, &log] (auto&& response) {
                LogResponse(log, response, total_time);
                send(response);
            };

            std::visit(send_response_and_log, std::move(response));
        }

    private:
        std::shared_ptr<http_handler::RequestHandler> decorated_;
    };
}
