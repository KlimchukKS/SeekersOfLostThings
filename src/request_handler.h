#pragma once

#include <boost/json.hpp>

#include "model.h"
#include "static_request_parser.h"
#include "api_request_parser.h"

namespace http_handler {
    namespace json = boost::json;

    class RequestHandler : public std::enable_shared_from_this<RequestHandler>  {
        using Strand = net::strand<net::io_context::executor_type>;
    public:
        explicit RequestHandler(model::Game& game,
                                std::filesystem::path&& static_path,
                                Strand strand,
                                extra_data::FrontendData&& frontend_data,
                                bool is_update_time_shift_automatic = false)
                : strand_{strand},
                api_parser_{game, is_update_time_shift_automatic, std::move(frontend_data)},
                static_request_parser_{std::move(static_path)} {

        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator>
        Response operator()(http::request<Body, http::basic_fields<Allocator>>&& req) {
            std::string query{ParseURL(std::string_view{req.target().data(), req.target().size()})};

            if (query.substr(0, ApiRequestType::api.size()) == ApiRequestType::api) {
                auto resp = std::make_shared<StringResponse>();

                auto h = [self = shared_from_this(), req = std::forward<decltype(req)>(req), &resp, &query] {
                    assert(self->strand_.running_in_this_thread());
                    *resp = self->api_parser_.ParseApiRequest(std::forward<decltype(req)>(req), std::move(query));
                };

                net::dispatch(strand_, h);

                return *resp;
            }

            return static_request_parser_.ParseFileRequest(std::forward<decltype(req)>(req), query);
        }

    private:
        Strand strand_;
        ApiRequestParser api_parser_;
        const StaticRequestParser static_request_parser_;

        static std::string ParseURL(std::string_view base_url);
    };
}  // namespace http_handler
