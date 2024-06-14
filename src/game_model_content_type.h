#pragma once

namespace model {

    template<typename StrType>
    struct GameModelContentType {
        GameModelContentType() = delete;

        constexpr static StrType maps{"maps"};
        constexpr static StrType default_dog_speed{"defaultDogSpeed"};

        constexpr static StrType id{"id"};
        constexpr static StrType name{"name"};
        constexpr static StrType dog_speed{"dogSpeed"};

        constexpr static StrType roads{"roads"};
        constexpr static StrType x0{"x0"};
        constexpr static StrType y0{"y0"};
        constexpr static StrType x1{"x1"};
        constexpr static StrType y1{"y1"};

        constexpr static StrType buildings{"buildings"};
        constexpr static StrType x{"x"};
        constexpr static StrType y{"y"};
        constexpr static StrType w{"w"};
        constexpr static StrType h{"h"};

        constexpr static StrType offices{"offices"};
        constexpr static StrType offsetX{"offsetX"};
        constexpr static StrType offsetY{"offsetY"};

        constexpr static StrType pos{"pos"};
        constexpr static StrType speed{"speed"};
        constexpr static StrType dir{"dir"};
        constexpr static StrType bag{"bag"};
        constexpr static StrType players{"players"};

        constexpr static StrType move{"move"};
        constexpr static StrType timeDelta{"timeDelta"};
        constexpr static StrType authToken{"authToken"};
        constexpr static StrType playerId{"playerId"};
        constexpr static StrType userName{"userName"};
        constexpr static StrType mapId{"mapId"};

        constexpr static StrType lootGeneratorConfig{"lootGeneratorConfig"};
        constexpr static StrType period{"period"};
        constexpr static StrType probability{"probability"};

        constexpr static StrType lootTypes{"lootTypes"};
        constexpr static StrType file{"file"};
        constexpr static StrType type{"type"};
        constexpr static StrType rotation{"rotation"};
        constexpr static StrType color{"color"};
        constexpr static StrType scale{"scale"};

        constexpr static StrType lostObjects{"lostObjects"};

        constexpr static StrType defaultBagCapacity{"defaultBagCapacity"};
        constexpr static StrType bagCapacity{"bagCapacity"};

        constexpr static StrType value{"value"};
        constexpr static StrType score{"score"};
    };

}