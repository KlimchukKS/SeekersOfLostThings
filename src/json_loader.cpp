#include "json_loader.h"

#include <boost/utility/string_view.hpp>

#include "game_model_content_type.h"

#include <fstream>

namespace json_loader {
    // Ключи для получения данных из JSON
    using gmct = model::GameModelContentType<boost::string_view>;

    void JsonLoader::SetRoads(Map& map, const json::value& roads) {
        for (const auto& road : roads.get_array()) {
            if (const auto* x1 = road.as_object().if_contains(gmct::x1)) {
                map.AddRoad(Road{Road::HORIZONTAL,
                                 Point{static_cast<int>(road.at(gmct::x0).as_int64()),
                                       static_cast<int>(road.at(gmct::y0).as_int64())},
                                 static_cast<int>(x1->as_int64())});
            } else if (const auto* y1 = road.as_object().if_contains(gmct::y1)) {
                map.AddRoad(Road{Road::VERTICAL,
                                 Point{static_cast<int>(road.at(gmct::x0).as_int64()),
                                       static_cast<int>(road.at(gmct::y0).as_int64())},
                                 static_cast<int>(y1->as_int64())});
            }
        }
    }

    void JsonLoader::SetBuildings(Map& map, const json::value& buildings) {
        for (auto& building : buildings.get_array()) {
            map.AddBuilding(
                    Building{
                            Rectangle{
                                    Point{static_cast<int>(building.at(gmct::x).as_int64()),
                                          static_cast<int>(building.at(gmct::y).as_int64())},
                                    Size{static_cast<int>(building.at(gmct::w).as_int64()),
                                         static_cast<int>(building.at(gmct::h).as_int64())}}});
        }
    }

    void JsonLoader::SetOffices(Map& map, const json::value& offices) {
        for (auto& office : offices.get_array()) {
            map.AddOffice(Office{Office::Id{office.at(gmct::id).as_string().data()},
                                 Point{static_cast<int>(office.at(gmct::x).as_int64()),
                                       static_cast<int>(office.at(gmct::y).as_int64())},
                                 Offset{static_cast<int>(office.at(gmct::offsetX).as_int64()),
                                        static_cast<int>(office.at(gmct::offsetY).as_int64())}});
        }
    }

    void JsonLoader::SetMaps(Game& game) {
        for (auto& maps : config_data_.at(gmct::maps).get_array()) {
            double dog_speed = game.GetDefaultDogSpeed();

            if (const auto* dog_speed_ptr = maps.as_object().if_contains(gmct::dog_speed)) {
                dog_speed = dog_speed_ptr->as_double();
            }

            unsigned bag_capacity = game.GetDefaultBagCapacity();

            if (const auto* bag_capacity_ptr = maps.as_object().if_contains(gmct::bagCapacity)) {
                bag_capacity = bag_capacity_ptr->as_uint64();
            }

            Map::LootTypeToValue loot_type_to_value;
            size_t loot_type_counter = 0;

            for (auto loot_type : maps.at(gmct::lootTypes).as_array()) {
                loot_type_to_value[loot_type_counter++] = loot_type.at(gmct::value).as_int64();
            }

            Map map{Map::Id{maps.at(gmct::id).as_string().data()},
                    maps.at(gmct::name).as_string().data(),
                    dog_speed,
                    std::move(loot_type_to_value),
                    bag_capacity};

            SetRoads(map, maps.as_object().at(gmct::roads));

            SetBuildings(map, maps.as_object().at(gmct::buildings));

            SetOffices(map, maps.as_object().at(gmct::offices));

            game.AddMap(std::move(map));
        }
    }

    std::string JsonLoader::GetFileStr(const std::filesystem::path& json_path) {
        if (!std::filesystem::exists(json_path) || !json_path.has_filename()) {
            throw std::runtime_error("The path leads to a file that does not exist");
        }

        std::ifstream file(json_path);

        if (!file) {
            throw std::runtime_error("Couldn't open the file");
        }

        std::stringstream input;
        input << file.rdbuf();
        file.close();

        return std::move(input.str());
    }

    model::Game JsonLoader::LoadGame() {
        Game game;

        if (const auto* dog_speed_ptr = config_data_.if_contains(gmct::default_dog_speed)) {
            game.SetDefaultDogSpeed(dog_speed_ptr->as_double());
        }

        if (const auto* bag_capacity_ptr = config_data_.if_contains(gmct::defaultBagCapacity)) {
            game.SetDefaultBagCapacity(bag_capacity_ptr->as_uint64());
        }

        {
            auto& loot_generator_config = config_data_.at(gmct::lootGeneratorConfig);
            game.SetLootGeneratorConfig(loot_generator_config.as_object().at(gmct::period).as_double(),
                                        loot_generator_config.as_object().at(gmct::probability).as_double());
        }

        SetMaps(game);

        return game;
    }

    extra_data::FrontendData JsonLoader::LoadFrontendData() {
        extra_data::FrontendData frontend_data;

        for (auto& map : config_data_.at(gmct::maps).get_array()) {
            frontend_data[Map::Id{map.at(gmct::id).as_string().data()}] = map.as_object().at(gmct::lootTypes).as_array();
        }

        return frontend_data;
    }

}  // namespace json_loader