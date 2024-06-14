#pragma once

#include "tagged.h"
#include "loot_generator.h"
#include "collision_detector.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <random>
#include <optional>
#include <deque>
#include <stack>
#include <tuple>

namespace model {

    using Dimension = int;
    using Coord = Dimension;

    using namespace std::string_literals;

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    class Road {
        struct HorizontalTag {
            HorizontalTag() = default;
        };

        struct VerticalTag {
            VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
                : start_{start}
                , end_{end_x, start.y} {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
                : start_{start}
                , end_{start.x, end_y} {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept {
            return start_;
        }

        Point GetEnd() const noexcept {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept
                : bounds_{bounds} {
        }

        const Rectangle& GetBounds() const noexcept {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
                : id_{std::move(id)}
                , position_{position}
                , offset_{offset} {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        Point GetPosition() const noexcept {
            return position_;
        }

        Offset GetOffset() const noexcept {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;
        using LootTypeToValue = std::unordered_map<size_t, size_t>;

        Map(Id id, std::string name, double dog_speed, LootTypeToValue&& loot_type_to_value, unsigned bag_capacity) noexcept
                : id_(std::move(id))
                , name_(std::move(name))
                , dog_speed_(dog_speed)
                , loot_type_to_value_(std::move(loot_type_to_value))
                , bag_capacity_(bag_capacity) {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }

        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }

        const Roads& GetRoads() const noexcept {
            return roads_;
        }

        const Offices& GetOffices() const noexcept {
            return offices_;
        }

        double GetDogSpeed() const noexcept {
            return dog_speed_;
        }

        unsigned GetBagCapacity() const noexcept {
            return bag_capacity_;
        }

        void AddRoad(const Road& road) {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building& building) {
            buildings_.emplace_back(building);
        }

        template <typename Arg>
        void AddOffice(Arg&& office) {
            if (warehouse_id_to_index_.contains(office.GetId())) {
                throw std::invalid_argument("Duplicate warehouse");
            }

            const size_t index = offices_.size();
            Office& o = offices_.emplace_back(std::forward<Arg>(office));
            try {
                warehouse_id_to_index_.emplace(o.GetId(), index);
            } catch (...) {
                // Удаляем офис из вектора, если не удалось вставить в unordered_map
                offices_.pop_back();
                throw;
            }
        }

        size_t GetAmountOfLootTypes() const {
            return loot_type_to_value_.size();
        }

        size_t GetLootTypeValue(size_t type) const {
            return loot_type_to_value_.at(type);
        }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;

        Roads roads_;
        Buildings buildings_;

        double dog_speed_;

        unsigned bag_capacity_;

        LootTypeToValue loot_type_to_value_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
    };

    struct Position {
        double x = 0.;
        double y = 0.;
    };

    struct Speed {
        double horizontal = 0.;
        double vertical = 0.;
    };

    enum class Direction {
        LEFT,
        RIGHT,
        UP,
        DOWN,
        STOP
    };

    class Dog {
    public:
        using MovementParameters = std::tuple<Direction, Position, Speed>;
        using LootsIdAndType = std::vector<std::pair<size_t, size_t>>;

        Dog(std::string&& name, unsigned id)
                : name_(std::move(name)), id_(id) {

        }

        [[nodiscard]] std::string_view GetName() const {
            return name_;
        }

        unsigned GetId() const {
            return id_;
        }

        MovementParameters GetMovementParameters() const {
            return std::make_tuple(dir_, pos_, speed_);
        }

        Direction GetDirection() {
            return dir_;
        }

        Position GetPosition() const {
            return pos_;
        }

        void SetMovementParameters(Direction dir, double default_dog_speed);

        void SetPosition(Position pos) {
            pos_ = pos;
        }

        void AddToBackpack(size_t loot_id, size_t loot_type) {
            bag_.emplace_back(std::make_pair(loot_id, loot_type));
        }

        void EmptyTheBackpack() {
            bag_.clear();
        }

        const LootsIdAndType& GetBackpackContents() const {
            return bag_;
        }

        void AddToTheScore(unsigned value) {
            score += value;
        }

        unsigned GetScore() const {
            return score;
        }

    private:
        const std::string name_;
        const unsigned id_;

        Direction dir_ = Direction::UP;
        Position pos_;
        Speed speed_;
        unsigned score = 0;

        LootsIdAndType bag_;
    };

    struct Loot {
        size_t type;
        size_t value;
        Position pos;
    };

    class GameSession {
    public:
        using LostObjectsIdToLoot = std::unordered_map<size_t, Loot>;

        GameSession(const Map* map, bool spawn_points_are_random, double period, double probability);

        void AddDog(Dog* dog);

        const std::vector<Dog*>& GetDogs() const {
            return dogs_;
        }

        const Map* GetMap() const {
            return map_;
        }

        void SetTimeShift(double shift_time);

        static bool CheckEqualityDouble(double lhs, double rhs);

        static bool LessOrEqual(double lhs, double rhs);

        static int RoundCoord(double coord);

        static std::pair<int, int> RoundCoordinates(double lhs, double rhs);

        const LostObjectsIdToLoot& GetLostObjects() const {
            return lost_objects_;
        }
    private:
        using Items = std::vector<collision_detector::Item>;
        using Gatherers = std::vector<collision_detector::Gatherer>;

        using ItemsIdToLostObjectsId = std::unordered_map<size_t , size_t>;
        using GatherIdToDog = std::unordered_map<size_t , Dog*>;

        const Map* map_;
        std::vector<Dog*> dogs_;

        loot_gen::LootGenerator loot_generator_;

        size_t lost_objects_id_counter = 0;
        LostObjectsIdToLoot lost_objects_;

        bool spawn_points_are_random_;

        constexpr static double distance_from_road_axis_to_boundary_ = 0.4;

        std::deque<Road> directed_roads_;
        std::vector<const Road*> directed_ptr_roads_;
        std::unordered_map<const Dog*, const Road*> dog_and_his_road_;
        const Road* starting_road_;

        struct PointHasher {
            size_t operator() (const std::pair<int, int>& value) const;
        private:
            std::hash<int> hasher_;
        };

        //Таблица позиций от которых условно начинаются и заканчиваются дороги
        std::unordered_map<std::pair<int, int>, const Road*, PointHasher> point_to_right_road_;
        std::unordered_map<std::pair<int, int>, const Road*, PointHasher> point_to_left_road_;
        std::unordered_map<std::pair<int, int>, const Road*, PointHasher> point_to_up_road_;
        std::unordered_map<std::pair<int, int>, const Road*, PointHasher> point_to_down_road_;

        std::pair<Items, ItemsIdToLostObjectsId> GetItemsAndItemsIdToLostObjectsId();
        // Так же, данная функция вызывает перемещение собак по карте
        std::pair<Gatherers, GatherIdToDog> GetGatherersAndGatherIdToDog(double shift_time);

        void SetTimeShiftForOneDog(double shift_time, Dog* dog);

        void MoveDogRight(double shift_time, Dog* dog);

        void MoveDogLeft(double shift_time, Dog* dog);

        void MoveDogUp(double shift_time, Dog* dog);

        void MoveDogDown(double shift_time, Dog* dog);

        int GetRandomNumberFromRange(int min, int max);

        void GenerateLostObjects(double shift_time);
    };

    class Player {
        struct TokenTag {};
    public:
        using Token = util::Tagged<std::string, TokenTag>;

        Player(std::string&& name, unsigned id, GameSession* session, Token&& token)
                : dog_{std::move(name), id}, session_{session}, token_{std::move(token)} {
        }

        const Dog* GetDog() const {
            return &dog_;
        }

        Dog* GetDog() {
            return &dog_;
        }

        const Token& GetToken() {
            return token_;
        }

        const GameSession* GetGameSession() const {
            return session_;
        }

        void SetDogMovementParameters(Direction dir) {
            dog_.SetMovementParameters(dir, session_->GetMap()->GetDogSpeed());
        }

    private:
        Dog dog_;
        const GameSession* session_;
        const Token token_;
    };

    class Players {
    public:
        std::pair<Player::Token, unsigned > AddPlayer(std::string&& name, GameSession* session);

        const Player* FindByToken(const Player::Token& token) const;

        Player* FindByToken(const Player::Token& token);

        const Player* FindByDogIdAndMapId(const std::pair<unsigned, Map::Id>& value) const;

    private:
        struct DogIdAndMapIdHasher {
            size_t operator() (const std::pair<unsigned, Map::Id>& value) const;

        private:
            std::hash<unsigned> unsigned_hasher_;
            util::TaggedHasher<Map::Id> map_id_hasher;
        };

        using TokenHasher = util::TaggedHasher<Player::Token>;
        using TokenToPlayer = std::unordered_map<Player::Token, Player*, TokenHasher>;
        using DogIdAndMapIdToPlayer = std::unordered_map<std::pair<unsigned, Map::Id>, Player*, DogIdAndMapIdHasher>;
        using PlayerToGameSession = std::unordered_map<Player*, GameSession*>;

        unsigned id_ = 0u;

        std::deque<Player> players_;
        std::unordered_map<unsigned, Player*> id_to_player_;

        TokenToPlayer token_to_player_;
        DogIdAndMapIdToPlayer dog_id_and_map_id_to_player_;

        static std::string GetPlayerToken();
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        template <typename Arg>
        void AddMap(Arg&& map) {
            const size_t index = maps_.size();
            if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
                throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
            } else {
                try {
                    maps_.emplace_back(std::forward<Arg>(map));
                } catch (...) {
                    map_id_to_index_.erase(it);
                    throw;
                }
            }
        }

        const Maps& GetMaps() const noexcept;

        const Map* FindMap(const Map::Id& id) const noexcept;

        const Players& GetPlayers() noexcept;

        const Player* FindPlayer(const Player::Token& token) const;

        Player* FindPlayer(const Player::Token& token);

        std::optional<std::pair<Player::Token, unsigned >> AddPlayer(std::string&& name, Map::Id&& id);

        void SetDefaultDogSpeed(double dog_speed);

        double GetDefaultDogSpeed();

        void SetTimeShift(double shift_time);

        void SetSpawnPointsRandom(bool spawn_points_are_random);

        void SetLootGeneratorConfig(double period, double probability);

        void SetDefaultBagCapacity(unsigned bag_capacity);

        unsigned GetDefaultBagCapacity();

        using MapIdHasher = util::TaggedHasher<Map::Id>;
    private:
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
        using MapIdToGameSessions = std::unordered_map<Map::Id, GameSession*, MapIdHasher>;

        double default_dog_speed_ = 1.0;
        bool spawn_points_are_random_ = false;

        double period_;
        double probability_;

        unsigned default_bag_capacity_ = 3;

        std::vector<Map> maps_;
        std::deque<GameSession> game_sessions_;
        MapIdToIndex map_id_to_index_;

        MapIdToGameSessions map_id_to_game_sessions_;
        Players players_;
    };

}  // namespace model