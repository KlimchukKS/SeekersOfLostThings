#pragma once

#include <boost/json.hpp>

#include "model.h"
#include "extra_data.h"

#include <filesystem>

namespace json_loader {

    namespace json = boost::json;
    using namespace model;

    class JsonLoader {
    public:
        JsonLoader(const std::filesystem::path& json_path)
        : config_data_{std::move(json::parse(GetFileStr(json_path)).as_object())} {
        }

        model::Game LoadGame();

        extra_data::FrontendData LoadFrontendData();

    private:
        json::object config_data_;

        static std::string GetFileStr(const std::filesystem::path& json_path);

        static void SetRoads(Map& map, const json::value& roads);

        static void SetBuildings(Map& map, const json::value& buildings);

        static void SetOffices(Map& map, const json::value& offices);

        void SetMaps(Game& game);
    };

}  // namespace json_loader