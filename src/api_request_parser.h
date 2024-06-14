#pragma once

#include <boost/json.hpp>

#include "model.h"
#include "http_handler_string_constants.h"
#include "response_maker.h"
#include "game_model_content_type.h"
#include "extra_data.h"

#include <unordered_map>

namespace http_handler {
    namespace net = boost::asio;

    namespace json = boost::json;

    class ApiRequestParser {
    public:
        // Ключи игровой модели для JSON
        using gmct = model::GameModelContentType<boost::string_view>;

        explicit ApiRequestParser(model::Game& game,
                                  bool is_update_time_shift_automatic,
                                  extra_data::FrontendData&& frontend_data)
                : game_(game)
                , frontend_data_{std::move(frontend_data)} {
            query_to_parser_type_.insert({ApiRequestType::join, ParserType::join});
            query_to_parser_type_.insert({ApiRequestType::players, ParserType::players});
            query_to_parser_type_.insert({ApiRequestType::state, ParserType::state});
            query_to_parser_type_.insert({ApiRequestType::action, ParserType::action});

            if (!is_update_time_shift_automatic) {
                query_to_parser_type_.insert({ApiRequestType::tick, ParserType::tick});
            }
        }

        ApiRequestParser(const ApiRequestParser&) = delete;
        ApiRequestParser& operator=(const ApiRequestParser&) = delete;

        template <typename Body, typename Allocator>
        StringResponse ParseApiRequest(const http::request<Body, http::basic_fields<Allocator>>& req, std::string&& query) {
            if (query.substr(0, std::min(ApiRequestType::maps.size(), query.size())) == ApiRequestType::maps) {
                return ParseMapsQuery(std::forward<decltype(req)>(req), query);
            }

            if (query_to_parser_type_.count(query)) {
                switch (query_to_parser_type_.at(query)) {
                    case ParserType::join:
                        return ParseJoinQuery(std::forward<decltype(req)>(req));
                    case ParserType::players:
                        return ParsePlayersQuery(std::forward<decltype(req)>(req));
                    case ParserType::state:
                        return ParseStateQuery(std::forward<decltype(req)>(req));
                    case ParserType::action:
                        return ParseActionQuery(std::forward<decltype(req)>(req));
                    case ParserType::tick:
                        return ParseTickQuery(std::forward<decltype(req)>(req));
                }
            }

            return MakeStringResponse(http::status::bad_request,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      ErrorMessages::badRequest);
        }

    private:
        model::Game& game_;

        extra_data::FrontendData frontend_data_;

        enum class ParserType {
            join,
            players,
            state,
            action,
            tick
        };

        std::unordered_map<model::Direction, std::string_view> direction_to_strv_{{model::Direction::UP, "U"},
                                                                                  {model::Direction::LEFT, "L"},
                                                                                  {model::Direction::RIGHT, "R"},
                                                                                  {model::Direction::DOWN, "D"},
                                                                                  {model::Direction::STOP, ""}};

        std::unordered_map<std::string_view, model::Direction> strv_to_direction_{{"L", model::Direction::LEFT},
                                                                                  {"R", model::Direction::RIGHT},
                                                                                  {"U", model::Direction::UP},
                                                                                  {"D", model::Direction::DOWN},
                                                                                  { "", model::Direction::STOP}};

        std::unordered_map<std::string_view, ParserType> query_to_parser_type_;

        template <typename Body, typename Allocator>
        StringResponse ParseMapsQuery(const http::request<Body, http::basic_fields<Allocator>>& req, std::string_view query) const {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                return MakeMethodNotAllowedResponse(http::status::method_not_allowed,
                                                    req.version(),
                                                    req.keep_alive(),
                                                    ContentType::APPLICATION_JSON,
                                                    ErrorMessages::invalidMethod,
                                                    "GET, HEAD");
            }

            if (query.size() == ApiRequestType::maps.size()) {
                return MakeStringResponse(http::status::ok,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          json::serialize(GetMapsJson()));
            }

            if (auto map = GetMap(ParseQueryMapName(query))) {
                return MakeStringResponse(http::status::ok,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          json::serialize(GetMapJson(map)));
            }

            return MakeStringResponse(http::status::not_found,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      ErrorMessages::mapNotFound);
        }

        template <typename Body, typename Allocator>
        StringResponse ParseJoinQuery(const http::request<Body, http::basic_fields<Allocator>>& req) {
            if (req.method() != http::verb::post) {
                return MakeMethodNotAllowedResponse(http::status::method_not_allowed,
                                                    req.version(),
                                                    req.keep_alive(),
                                                    ContentType::APPLICATION_JSON,
                                                    ErrorMessages::invalidMethodApiJoin,
                                                    "POST");
            }

            std::string userName;
            std::string mapId;
            try {
                json::value player_data = json::parse(req.body());
                userName = player_data.at(gmct::userName).as_string().data();
                mapId = player_data.at(gmct::mapId).as_string().data();
            } catch(...) {
                return MakeStringResponse(http::status::bad_request,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidArgumentApiJoinJson);
            }

            if (userName.empty()) {
                return MakeStringResponse(http::status::bad_request,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidArgumentApiJoinName);
            }

            auto add_player_result = game_.AddPlayer(std::move(userName), model::Map::Id{std::move(mapId)});

            if (!add_player_result) {
                return MakeStringResponse(http::status::not_found,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::mapNotFound);
            }
            auto [authToken, playerId] = add_player_result.value();

            json::object response_json{{gmct::authToken, *authToken}, {gmct::playerId, playerId}};

            return MakeStringResponse(http::status::ok,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      json::serialize(response_json));
        }

        template <typename Body, typename Allocator>
        StringResponse ParsePlayersQuery(const http::request<Body, http::basic_fields<Allocator>>& req) {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                return MakeMethodNotAllowedResponse(http::status::method_not_allowed,
                                                    req.version(),
                                                    req.keep_alive(),
                                                    ContentType::APPLICATION_JSON,
                                                    ErrorMessages::invalidMethod,
                                                    "GET, HEAD");
            }

            std::string user_token = ParseBearer(req[http::field::authorization]);

            if (user_token.empty()) {
                return MakeStringResponse(http::status::unauthorized,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidToken);
            }

            const model::Player* player;

            if (!(player = game_.FindPlayer(model::Player::Token{std::move(user_token)}))) {
                return MakeStringResponse(http::status::unauthorized,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::unknownToken);
            }

            return MakeStringResponse(http::status::ok,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      json::serialize(GetDogsList(player->GetGameSession()->GetDogs())));
        }

        template <typename Body, typename Allocator>
        StringResponse ParseStateQuery(const http::request<Body, http::basic_fields<Allocator>>& req) {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                return MakeMethodNotAllowedResponse(http::status::method_not_allowed,
                                                    req.version(),
                                                    req.keep_alive(),
                                                    ContentType::APPLICATION_JSON,
                                                    ErrorMessages::invalidMethod,
                                                    "GET, HEAD");
            }

            std::string user_token = ParseBearer(req[http::field::authorization]);

            if (user_token.empty()) {
                return MakeStringResponse(http::status::unauthorized,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidToken);
            }

            const model::Player* player;

            if (!(player = game_.FindPlayer(model::Player::Token{std::move(user_token)}))) {
                return MakeStringResponse(http::status::unauthorized,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::unknownToken);
            }

            return MakeStringResponse(http::status::ok,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      json::serialize(GetGameState(player->GetGameSession())));
        }

        template <typename Body, typename Allocator>
        StringResponse ParseActionQuery(const http::request<Body, http::basic_fields<Allocator>>& req) {
            if (req.method() != http::verb::post) {
                return MakeMethodNotAllowedResponse(http::status::method_not_allowed,
                                                    req.version(),
                                                    req.keep_alive(),
                                                    ContentType::APPLICATION_JSON,
                                                    ErrorMessages::invalidMethodApiJoin,
                                                    "POST");
            }
            //Проверяем токен
            std::string user_token = ParseBearer(req[http::field::authorization]);

            if (user_token.empty()) {
                return MakeStringResponse(http::status::unauthorized,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidToken);
            }

            model::Player* player;

            if (!(player = game_.FindPlayer(model::Player::Token{std::move(user_token)}))) {
                return MakeStringResponse(http::status::unauthorized,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::unknownToken);
            }

            model::Direction dir;
            try {
                json::value player_data = json::parse(req.body());
                dir = strv_to_direction_.at(player_data.at(gmct::move).as_string().data());
            } catch(...) {
                return MakeStringResponse(http::status::bad_request,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidArgumentToParseAction);
            }

            player->SetDogMovementParameters(dir);

            return MakeStringResponse(http::status::ok,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      "{}");
        }

        template <typename Body, typename Allocator>
        StringResponse ParseTickQuery(const http::request<Body, http::basic_fields<Allocator>>& req) {
            if (req.method() != http::verb::post) {
                return MakeMethodNotAllowedResponse(http::status::method_not_allowed,
                                                    req.version(),
                                                    req.keep_alive(),
                                                    ContentType::APPLICATION_JSON,
                                                    ErrorMessages::invalidMethodApiJoin,
                                                    "POST");
            }

            double time_delta;

            try {
                json::value player_data = json::parse(req.body());

                if (player_data.at(gmct::timeDelta).is_int64()) {
                    time_delta = player_data.at(gmct::timeDelta).as_int64();
                } else {
                    time_delta = player_data.at(gmct::timeDelta).as_double();
                }
                if (time_delta < 1e-6) {
                    throw 1;
                }
            } catch(...) {
                return MakeStringResponse(http::status::bad_request,
                                          req.version(),
                                          req.keep_alive(),
                                          ContentType::APPLICATION_JSON,
                                          ErrorMessages::invalidArgumentToParseJSON);
            }

            // В аргументе преобразуем секунды в миллисекунды
            game_.SetTimeShift(time_delta / 1000.);

            return MakeStringResponse(http::status::ok,
                                      req.version(),
                                      req.keep_alive(),
                                      ContentType::APPLICATION_JSON,
                                      "{}");
        }

        [[nodiscard]] const model::Map* GetMap(const std::string& map_name) const;

        static std::string ParseQueryMapName(std::string_view query);

        json::value GetMapsJson() const;

        json::value GetMapJson(const model::Map* map) const;

        static json::array GetRoads(const model::Map* map);

        static json::array GetBuildings(const model::Map* map);

        static json::array GetOffices(const model::Map* map);

        static json::value GetDogsList(const std::vector<model::Dog*>& dogs);

        json::array GetPlayerBag(const model::Dog* dog) const;

        json::object GetPlayersToState(const std::vector<model::Dog*>& dogs) const;

        json::value GetGameState(const model::GameSession* game_session) const;

        static std::string ParseBearer(std::string_view query);
    };
}