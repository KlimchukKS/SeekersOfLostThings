#include "api_request_parser.h"

namespace http_handler {

    std::string ApiRequestParser::ParseQueryMapName(std::string_view query) {
        std::string_view root = query.substr(0, ApiRequestType::maps.size());

        if (query.size() > (ApiRequestType::maps.size() + 1) && root == ApiRequestType::maps) {
            query.remove_prefix(ApiRequestType::maps.size() + 1);
            if (query.find('/') == std::string::npos) {
                return std::string{query.begin(), query.end()};
            }
        }

        return "";
    }

    const model::Map* ApiRequestParser::GetMap(const std::string& map_name) const {
        if (!map_name.empty()) {
            return game_.FindMap(model::Map::Id{map_name});
        }
        return nullptr;
    }

    json::array ApiRequestParser::GetRoads(const model::Map* map) {
        json::array roads;

        for (auto& road : map->GetRoads()) {
            auto start = road.GetStart();

            auto is_horizontal = road.IsHorizontal();

            roads.emplace_back(json::object{ {gmct::x0, start.x},
                                             {gmct::y0, start.y},
                                             {((is_horizontal) ? gmct::x1 : gmct::y1),
                                              ((is_horizontal) ? road.GetEnd().x : road.GetEnd().y)} });
        }

        return roads;
    }

    json::array ApiRequestParser::GetBuildings(const model::Map* map) {
        json::array buildings;

        for (auto& building : map->GetBuildings()) {
            auto bounds = building.GetBounds();

            buildings.emplace_back(json::object{{gmct::x, bounds.position.x},
                                                {gmct::y, bounds.position.y},
                                                {gmct::w, bounds.size.width},
                                                {gmct::h, bounds.size.height}});
        }

        return buildings;
    }

    json::array ApiRequestParser::GetOffices(const model::Map* map) {
        json::array offices;
        for (auto& office : map->GetOffices()) {
            offices.emplace_back(json::object{{gmct::id, *office.GetId()},
                                              {gmct::x, office.GetPosition().x},
                                              {gmct::y, office.GetPosition().y},
                                              {gmct::offsetX, office.GetOffset().dx},
                                              {gmct::offsetY, office.GetOffset().dy}});
        }

        return offices;
    }

    json::value ApiRequestParser::GetMapsJson() const {
        json::array maps_json;

        for (auto& map : game_.GetMaps()) {
            json::object obj;
            obj.insert({{gmct::id, *map.GetId()},
                        {gmct::name,map.GetName()}});
            maps_json.emplace_back(std::move(obj));
        }

        return maps_json;
    }

    json::value ApiRequestParser::GetMapJson(const model::Map* map) const {
        json::value map_json;

        map_json.emplace_object().insert({{gmct::id, *map->GetId()},
                                          {gmct::name, map->GetName()},
                                          {gmct::roads, std::move(GetRoads(map))},
                                          {gmct::buildings, std::move(GetBuildings(map))},
                                          {gmct::offices, std::move(GetOffices(map))},
                                          {gmct::lootTypes, frontend_data_.at(map->GetId())}});

        return map_json;
    }

    json::value ApiRequestParser::GetDogsList(const std::vector<model::Dog*>& dogs) {
        json::object players;

        for (const model::Dog* dog :  dogs) {
            boost::string_view tmp = std::to_string(dog->GetId());
            players.insert({{tmp, json::object{{{gmct::name, dog->GetName()}}}}});
        }

        return players;
    }

    json::array ApiRequestParser::GetPlayerBag(const model::Dog* dog) const {
        json::array bag;

        for (auto [id, type] : dog->GetBackpackContents()) {
            json::object obj;
            obj.insert({{gmct::id, id},
                        {gmct::type, type}});

            bag.emplace_back(std::move(obj));
        }

        return bag;
    }

    json::object ApiRequestParser::GetPlayersToState(const std::vector<model::Dog*>& dogs) const {
        json::object players;

        for (const model::Dog* dog :  dogs) {
            boost::string_view tmp = std::to_string(dog->GetId());

            auto [dir, pos, speed] = dog->GetMovementParameters();

            players.insert({{tmp, json::object{{{gmct::pos, json::array{pos.x, pos.y}},
                                                {gmct::speed, json::array{speed.horizontal, speed.vertical}},
                                                {gmct::dir, direction_to_strv_.at(dir)},
                                                {gmct::bag, GetPlayerBag(dog)},
                                                {gmct::score, dog->GetScore()}}}}});
        }

        return players;
    }

    json::value ApiRequestParser::GetGameState(const model::GameSession* game_session) const {
        json::object lost_objects;

        for (const auto& [id, loot] : game_session->GetLostObjects()) {
            boost::string_view tmp = std::to_string(id);

            lost_objects.insert({{tmp, json::object{{gmct::type, loot.type},
                                                    {gmct::pos, json::array{loot.pos.x, loot.pos.y}}}}});
        }

        json::object ans;
        ans.insert({{gmct::players, GetPlayersToState(game_session->GetDogs())},
                    {gmct::lostObjects, std::move(lost_objects)}});

        return ans;
    }

    std::string ApiRequestParser::ParseBearer(std::string_view query) {
        constexpr static std::string_view kBearer{"Bearer "sv};

        if (query.substr(0, kBearer.size()) != kBearer) {
            return "";
        }

        query.remove_prefix(std::min(kBearer.size(), query.size()));

        if (query.size() != 32) {
            return "";
        }

        return {query.data(), query.size()};
    }
}
