#include "model.h"

#include <iomanip>

namespace model {

    void Dog::SetMovementParameters(Direction dir, double default_dog_speed) {
        switch (dir) {
            case Direction::UP:
                speed_.horizontal = 0.; speed_.vertical = -default_dog_speed;
                break;
            case Direction::DOWN:
                speed_.horizontal = 0.; speed_.vertical = default_dog_speed;
                break;
            case Direction::LEFT:
                speed_.horizontal = -default_dog_speed; speed_.vertical = 0.;
                break;
            case Direction::RIGHT:
                speed_.horizontal = default_dog_speed; speed_.vertical = 0.;
                break;
            case Direction::STOP:
                speed_.horizontal = 0.; speed_.vertical = 0.;
                return;
        }
        dir_ = dir;
    }

    //Определения методов класса GameSession
    using namespace std::chrono_literals;
    GameSession::GameSession(const Map* map, bool spawn_points_are_random, double period, double probability)
            : map_(map)
            , spawn_points_are_random_(spawn_points_are_random)
            , loot_generator_(std::chrono::duration_cast<std::chrono::milliseconds>(period * 1ms), probability)
    {
        for(const auto& road : map->GetRoads()) {
            auto point_start = std::make_pair(road.GetStart().x, road.GetStart().y);
            auto point_end = std::make_pair(road.GetEnd().x, road.GetEnd().y);
            if (road.IsHorizontal()) {
                //Приводим дороги к виду при котором стартовая координата всегда меньше конечной
                if (road.GetStart().x < road.GetEnd().x) {
                    directed_roads_.emplace_back(Road{Road::HORIZONTAL, road.GetStart(), road.GetEnd().x});
                    directed_ptr_roads_.push_back(&directed_roads_.back());
                    point_to_right_road_.insert({point_start, &directed_roads_.back()});
                    point_to_left_road_.insert({point_end, &directed_roads_.back()});
                } else {
                    //Тут как-бы переворачиваем дору
                    directed_roads_.emplace_back(Road{Road::HORIZONTAL, road.GetEnd(), road.GetStart().x});
                    directed_ptr_roads_.push_back(&directed_roads_.back());
                    point_to_right_road_.insert({point_end, &directed_roads_.back()});
                    point_to_left_road_.insert({point_start, &directed_roads_.back()});
                }
            } else {
                if (road.GetStart().y < road.GetEnd().y) {
                    directed_roads_.emplace_back(Road{Road::VERTICAL, road.GetStart(), road.GetEnd().y});
                    directed_ptr_roads_.push_back(&directed_roads_.back());
                    point_to_down_road_.insert({point_start, &directed_roads_.back()});
                    point_to_up_road_.insert({point_end, &directed_roads_.back()});
                } else {
                    directed_roads_.emplace_back(Road{Road::VERTICAL, road.GetEnd(), road.GetStart().y});
                    directed_ptr_roads_.push_back(&directed_roads_.back());
                    point_to_down_road_.insert({point_end, &directed_roads_.back()});
                    point_to_up_road_.insert({point_start, &directed_roads_.back()});
                }
            }
        }

        //В качестве начальной дороги, выбираем любую, с нулевыми координатами.
        starting_road_ = (point_to_right_road_.count({0,0})) ?
                         point_to_right_road_.at({0,0}) : point_to_down_road_.at({0,0});
    }

    void GameSession::AddDog(Dog* dog) {
        dogs_.push_back(dog);

        if (spawn_points_are_random_) {
            auto ptr_road = directed_ptr_roads_[GetRandomNumberFromRange(0, directed_ptr_roads_.size() - 1)];
            dog_and_his_road_[dog] = ptr_road;

            double x = GetRandomNumberFromRange(ptr_road->GetStart().x, ptr_road->GetEnd().x);
            double y = GetRandomNumberFromRange(ptr_road->GetStart().y, ptr_road->GetEnd().y);

            dog->SetPosition({x, y});
        } else {
            //Все собаки появляются на одной дороге у которой одна из точек в нулевых координатах
            dog_and_his_road_[dog] = starting_road_;
        }
    }

    void GameSession::GenerateLostObjects(double shift_time) {
        unsigned loot_to_generation = loot_generator_.Generate(std::chrono::duration_cast<std::chrono::milliseconds>(shift_time * 1ms),
                                                               lost_objects_.size(),
                                                               dogs_.size());

        for (unsigned i = 0; i < loot_to_generation; ++i) {
            Loot loot;
            loot.type = GetRandomNumberFromRange(0, map_->GetAmountOfLootTypes() - 1);

            auto ptr_road = directed_ptr_roads_[GetRandomNumberFromRange(0, directed_ptr_roads_.size() - 1)];
            loot.pos.x = GetRandomNumberFromRange(ptr_road->GetStart().x, ptr_road->GetEnd().x);
            loot.pos.y = GetRandomNumberFromRange(ptr_road->GetStart().y, ptr_road->GetEnd().y);

            lost_objects_[lost_objects_id_counter++] = loot;
        }
    }

    std::pair<GameSession::Items, GameSession::ItemsIdToLostObjectsId> GameSession::GetItemsAndItemsIdToLostObjectsId() {
        Items items;
        size_t items_id_counter = 0;

        // Добавляем офисы, чтобы проверить на посещение собакой офиса
        for(auto office : map_->GetOffices()) {
            auto pos = office.GetPosition();
            items.push_back({{static_cast<double>(pos.x), static_cast<double>(pos.y)}, 0.25});
            ++items_id_counter;
        }
        // Нам неважно, какой конкретно офис посещает собака, поэтому запоминаем только лут
        std::unordered_map<size_t , size_t> items_id_to_lost_objects_id;

        for (auto [id, loot] : lost_objects_) {
            auto pos = loot.pos;
            items.push_back({{static_cast<double>(pos.x), static_cast<double>(pos.y)}, 0.});

            items_id_to_lost_objects_id[items_id_counter++] = id;
        }

        return std::make_pair(std::move(items), std::move(items_id_to_lost_objects_id));
    }

    std::pair<GameSession::Gatherers, GameSession::GatherIdToDog> GameSession::GetGatherersAndGatherIdToDog(double shift_time) {
        Gatherers gatherers;

        std::unordered_map<size_t , Dog*> gather_id_to_dog;
        size_t gather_id_counter = 0;

        for (auto dog : dogs_) {
            // Устанавливаем стартовую позицию сборщику
            auto start_pos = dog->GetPosition();

            //Перемещаем собаку в конечную точку
            SetTimeShiftForOneDog(shift_time, dog);

            // Устанавливаем конечную позицию сборщика
            auto end_pos = dog->GetPosition();

            gather_id_to_dog[gather_id_counter++] = dog;
            gatherers.push_back({{start_pos.x, start_pos.y},{end_pos.x, end_pos.y},0.3});
        }

        return std::make_pair(std::move(gatherers), std::move(gather_id_to_dog));
    }

    void GameSession::SetTimeShift(double shift_time) {
        auto [gatherers, gather_id_to_dog] = GetGatherersAndGatherIdToDog(shift_time);

        auto [items, items_id_to_lost_objects_id] = GetItemsAndItemsIdToLostObjectsId();

        collision_detector::VectorItemGathererProvider provider(std::move(items), std::move(gatherers));

        auto gather_events = collision_detector::FindGatherEvents(provider);

        for (auto& event : gather_events) {
            auto dog = gather_id_to_dog[event.gatherer_id];
            // Если id ивента нет в словаре, то обрабатываем ивент посещения собакой базы
            if (!items_id_to_lost_objects_id.count(event.item_id)) {
                // Собака пришла на базу.
                // Производим подсчёт очков.
                for (auto [id, type] : dog->GetBackpackContents()) {
                    dog->AddToTheScore(map_->GetLootTypeValue(type));
                }

                // Опустошаем рюкзак
                dog->EmptyTheBackpack();
            }
            // Если предмет есть на карте. Предмета может не быть, поскольку другая собака могла его уже забрать
            else if (auto loot_id = items_id_to_lost_objects_id[event.item_id]; lost_objects_.count(loot_id)) {
                // Добавляем в рюкзак собаки предмет
                dog->AddToBackpack(loot_id, lost_objects_[loot_id].type);
                // Удаляем с карты предмет, чтобы другая собака в радиусе предмета его не подобрала
                lost_objects_.erase(loot_id);
            }
        }

        GenerateLostObjects(shift_time);
    }

    bool GameSession::CheckEqualityDouble(double lhs, double rhs) {
        constexpr double EPSILON = 1e-6;
        return (std::abs(lhs - rhs) < EPSILON);
    }

    bool GameSession::LessOrEqual(double lhs, double rhs) {
        return lhs < rhs || CheckEqualityDouble(lhs, rhs);
    }

    int GameSession::RoundCoord(double coord) {
        double num, den;
        den = std::modf(coord, &num);

        int pos = num;

        return (LessOrEqual(den, distance_from_road_axis_to_boundary_)) ? pos : ++pos;
    }

    std::pair<int, int> GameSession::RoundCoordinates(double lhs, double rhs) {
        return std::make_pair(RoundCoord(lhs), RoundCoord(rhs));
    }

    void GameSession::MoveDogRight(double shift_time, Dog* dog) {
        auto [dir, pos, speed] = dog->GetMovementParameters();

        double distance = shift_time * speed.horizontal;

        //Как-бы рекурсивно проходим все дороги по которым пройдет собака
        std::stack<const Road*> st;
        st.push(dog_and_his_road_.at(dog));

        while (!st.empty()) {
            st.pop();

            double road_edge_coordinate = dog_and_his_road_.at(dog)->GetEnd().x + distance_from_road_axis_to_boundary_;

            //Если дистанция перемещения не выходит из площади текущей дороги
            if (LessOrEqual(distance + pos.x, road_edge_coordinate)) {
                dog->SetPosition({distance + pos.x, pos.y});

                if (CheckEqualityDouble(distance + pos.x, road_edge_coordinate)) {
                    dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                }

                return;
            }

            //Иначе доходим до конца дороги, преодолев необходимую дистанцию
            distance -= road_edge_coordinate - pos.x;
            pos.x = road_edge_coordinate;

            //Если у нас больше нет дорог справа, а дистанция для преодоления осталась, то мы уперлись в границу карты
            auto dog_coord = RoundCoordinates(pos.x, pos.y);
            if (!point_to_right_road_.count(dog_coord)) {
                dog->SetPosition({pos.x, pos.y});
                dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                return;
            }
            //Добавляем в стек дорогу, на которую можем перейти и продолжаем движение
            st.push(point_to_right_road_.at(dog_coord));
            dog_and_his_road_[dog] = st.top();
        }
    }

    void GameSession::MoveDogLeft(double shift_time, Dog* dog) {
        auto [dir, pos, speed] = dog->GetMovementParameters();

        double distance = shift_time * speed.horizontal;

        //Как-бы рекурсивно проходим все дороги по которым пройдет собака
        std::stack<const Road*> st;
        st.push(dog_and_his_road_.at(dog));

        while (!st.empty()) {
            st.pop();

            double road_edge_coordinate = dog_and_his_road_.at(dog)->GetStart().x - distance_from_road_axis_to_boundary_;

            //Если дистанция перемещения не выходит из площади текущей дороги
            if (LessOrEqual(road_edge_coordinate, distance + pos.x)) {
                dog->SetPosition({distance + pos.x, pos.y});

                if (CheckEqualityDouble(distance + pos.x, road_edge_coordinate)) {
                    dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                }

                return;
            }

            //Иначе доходим до конца дороги, преодолев необходимую дистанцию
            distance -= road_edge_coordinate - pos.x;
            pos.x = road_edge_coordinate;

            //Если у нас больше нет дорог слева, а дистанция для преодоления осталась, то мы уперлись в границу карты
            auto dog_coord = RoundCoordinates(pos.x, pos.y);
            if (!point_to_left_road_.count(dog_coord)) {
                dog->SetPosition({pos.x, pos.y});
                dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                return;
            }
            //Добавляем в стек дорогу, на которую можем перейти и продолжаем движение
            st.push(point_to_left_road_.at(dog_coord));
            dog_and_his_road_[dog] = st.top();
        }
    }

    void GameSession::MoveDogUp(double shift_time, Dog* dog) {
        auto [dir, pos, speed] = dog->GetMovementParameters();

        double distance = shift_time * speed.vertical;

        //Как-бы рекурсивно проходим все дороги по которым пройдет собака
        std::stack<const Road*> st;
        st.push(dog_and_his_road_.at(dog));

        while (!st.empty()) {
            st.pop();

            double road_edge_coordinate = dog_and_his_road_.at(dog)->GetStart().y - distance_from_road_axis_to_boundary_;

            //Если дистанция перемещения не выходит из площади текущей дороги
            if (LessOrEqual(road_edge_coordinate, distance + pos.y)) {
                dog->SetPosition({pos.x, distance + pos.y});

                if (CheckEqualityDouble(distance + pos.y, road_edge_coordinate)) {
                    dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                }

                return;
            }

            //Иначе доходим до конца дороги, преодолев необходимую дистанцию
            distance -= road_edge_coordinate - pos.y;
            pos.y = road_edge_coordinate;

            //Если у нас больше нет дорог, а дистанция для преодоления осталась, то мы уперлись в границу карты
            auto dog_coord = RoundCoordinates(pos.x, pos.y);
            if (!point_to_up_road_.count(dog_coord)) {
                dog->SetPosition({pos.x, pos.y});
                dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                return;
            }
            //Добавляем в стек дорогу, на которую можем перейти и продолжаем движение
            st.push(point_to_up_road_.at(dog_coord));
            dog_and_his_road_[dog] = st.top();
        }
    }

    void GameSession::MoveDogDown(double shift_time, Dog* dog) {
        auto [dir, pos, speed] = dog->GetMovementParameters();

        double distance = shift_time * speed.vertical;

        //Как-бы рекурсивно проходим все дороги по которым пройдет собака
        std::stack<const Road*> st;
        st.push(dog_and_his_road_.at(dog));

        while (!st.empty()) {
            st.pop();

            double road_edge_coordinate = dog_and_his_road_.at(dog)->GetEnd().y + distance_from_road_axis_to_boundary_;

            //Если дистанция перемещения не выходит из площади текущей дороги
            if (LessOrEqual(distance + pos.y, road_edge_coordinate)) {
                dog->SetPosition({pos.x, distance + pos.y});

                if (CheckEqualityDouble(distance + pos.y, road_edge_coordinate)) {
                    dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                }

                return;
            }

            //Иначе доходим до конца дороги, преодолев необходимую дистанцию
            distance -= road_edge_coordinate - pos.y;
            pos.y = road_edge_coordinate;

            //Если у нас больше нет дорог справа, а дистанция для преодоления осталась, то мы уперлись в границу карты
            auto dog_coord = RoundCoordinates(pos.x, pos.y);
            if (!point_to_down_road_.count(dog_coord)) {
                dog->SetPosition({pos.x, pos.y});
                dog->SetMovementParameters(Direction::STOP, map_->GetDogSpeed());
                return;
            }
            //Добавляем в стек дорогу, на которую можем перейти и продолжаем движение
            st.push(point_to_down_road_.at(dog_coord));
            dog_and_his_road_[dog] = st.top();
        }
    }

    size_t GameSession::PointHasher::operator() (const std::pair<int, int>& value) const {
        size_t h_x = hasher_(value.first);
        size_t h_y = hasher_(value.second);

        return h_x + h_y * 1000000007; // Где литерал - это выбранная константа, необходима для уменьшения коллизий
    }

    void GameSession::SetTimeShiftForOneDog(double shift_time, Dog* dog) {
        switch (dog->GetDirection()) {
            case Direction::RIGHT:
                MoveDogRight(shift_time, dog);
                break;
            case Direction::LEFT:
                MoveDogLeft(shift_time, dog);
                break;
            case Direction::UP:
                MoveDogUp(shift_time, dog);
                break;
            case Direction::DOWN:
                MoveDogDown(shift_time, dog);
                break;
        }
    }

    int GameSession::GetRandomNumberFromRange(int min, int max) {
        std::random_device random_device_;

        std::uniform_int_distribution<> dist(min, max);

        return dist(random_device_);
    }

    //Определения методов класса Players

    std::pair<Player::Token, unsigned > Players::AddPlayer(std::string&& name, GameSession* session) {
        players_.emplace_back(Player{std::move(name), id_, session, Player::Token{GetPlayerToken()}});

        session->AddDog(players_.back().GetDog());
        id_to_player_.emplace(id_, &players_.back());

        token_to_player_.emplace(players_.back().GetToken(), &players_.back());

        dog_id_and_map_id_to_player_.emplace(std::make_pair(id_, session->GetMap()->GetId()), &players_.back());

        return std::make_pair(players_.back().GetToken(), id_++);
    }

    const Player* Players::FindByToken(const Player::Token& token) const {
        if (token_to_player_.count(token)) {
            return token_to_player_.at(token);
        }
        return nullptr;
    }

    Player* Players::FindByToken(const Player::Token& token) {
        if (token_to_player_.count(token)) {
            return token_to_player_.at(token);
        }
        return nullptr;
    }

    const Player* Players::FindByDogIdAndMapId(const std::pair<unsigned, Map::Id>& value) const {
        if (dog_id_and_map_id_to_player_.count(value)) {
            return dog_id_and_map_id_to_player_.at(value);
        }

        return nullptr;
    }

    std::string Players::GetPlayerToken() {
        std::random_device random_device_;

        std::uniform_int_distribution<std::mt19937_64::result_type> dist;

        std::string result;

        do {
            std::stringstream stream;
            stream << std::hex << dist(random_device_) << dist(random_device_);
            result = stream.str();
        } while (result.size() != 32); // Литерал - это допустимое количество символов в токене

        return result;
    }

    size_t Players::DogIdAndMapIdHasher::operator() (const std::pair<unsigned, Map::Id>& value) const {
        size_t h_x = unsigned_hasher_(value.first);
        size_t h_y = map_id_hasher(value.second);

        return h_x + h_y * 1000000007; // Где литерал - это выбранная константа, необходима для уменьшения коллизий
    }

    //Определения методов класса Game

    const Game::Maps& Game::GetMaps() const noexcept {
        return maps_;
    }

    const Map* Game::FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    const Players& Game::GetPlayers() noexcept {
        return players_;
    }

    const Player* Game::FindPlayer(const Player::Token& token) const {
        return players_.FindByToken(token);
    }

    Player* Game::FindPlayer(const Player::Token& token) {
        return players_.FindByToken(token);
    }

    std::optional<std::pair<Player::Token, unsigned >> Game::AddPlayer(std::string&& name, Map::Id&& id) {
        const Map* map = FindMap(id);
        if (!map) {
            return std::nullopt;
        }
        //На данный момент, на каждую карту, одна сессия
        if (!map_id_to_game_sessions_.count(id)) {
            game_sessions_.emplace_back(GameSession{map,
                                                    spawn_points_are_random_,
                                                    period_,
                                                    probability_});
            map_id_to_game_sessions_.emplace(id, &game_sessions_.back());
        }

        return players_.AddPlayer(std::move(name), map_id_to_game_sessions_.at(id)/*, id*/);
    }

    void Game::SetDefaultDogSpeed(double dog_speed) {
        default_dog_speed_ = dog_speed;
    }

    double Game::GetDefaultDogSpeed() {
        return default_dog_speed_;
    }

    void Game::SetDefaultBagCapacity(unsigned bag_capacity) {
        default_bag_capacity_ = bag_capacity;
    }

    unsigned Game::GetDefaultBagCapacity() {
        return default_bag_capacity_;
    }

    void Game::SetTimeShift(double shift_time) {
        for (auto& gs : game_sessions_) {
            gs.SetTimeShift(shift_time);
        }
    }

    void Game::SetSpawnPointsRandom(bool spawn_points_are_random) {
        spawn_points_are_random_ = spawn_points_are_random;
    }

    void Game::SetLootGeneratorConfig(double period, double probability) {
        period_ = period;
        probability_ = probability;
    }

}  // namespace model