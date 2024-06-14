#include "static_request_parser.h"

namespace http_handler {

    std::filesystem::path StaticRequestParser::GetFilePath(std::string_view query) const {
        auto get_another = [&query] (const std::filesystem::path& static_path) {
            query.remove_prefix(1);
            return std::filesystem::weakly_canonical(static_path / query);
        };
        //Файл с именем index может быть получен по двум именам из адресной строки запроса
        return (query == "/"sv || query == GetFileRequestType::index) ?
               std::filesystem::weakly_canonical(static_path_ / GetFileRequestType::index) : get_another(static_path_);
    }

    bool StaticRequestParser::IsSubPath(fs::path path, fs::path base) {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

}