#pragma once

#include <boost/json.hpp>

#include "model.h"

namespace extra_data {
    namespace json = boost::json;

    using FrontendData = std::unordered_map<model::Map::Id, json::array, model::Game::MapIdHasher>;
}
