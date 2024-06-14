#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json.hpp>

//Включения необходимые для логирования
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/date_time.hpp>

#include <boost/program_options.hpp>
#include <optional>

#include "json_loader.h"
#include "request_handler.h"
#include "server_logging.h"
#include "ticker.h"

#include <iostream>
#include <thread>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "data", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

namespace {

    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned thread_count, const Fn& fn) {
        thread_count = std::max(1u, thread_count);
        std::vector<std::jthread> workers;
        workers.reserve(thread_count - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--thread_count) {
            workers.emplace_back(fn);
        }
        fn();
    }

    struct Args {
        std::optional<long> milliseconds;
        std::string file;
        std::string dir;
        bool spawn_points_are_random;
    };

    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
        namespace po = boost::program_options;

        po::options_description desc{"All options"s};

        std::string milliseconds;
        Args args;
        desc.add_options()
                ("help,h", "produce help message")
                ("tick-period,t", po::value(&milliseconds)->value_name("milliseconds"s), "set tick period")
                ("config-file,c", po::value(&args.file)->value_name("file"s), "set config file path")
                ("www-root,w", po::value(&args.dir)->value_name("dir"s), "set static files root")
                ("randomize-spawn-points", "spawn dogs at random positions");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.contains("help"s)) {
            // Если был указан параметр --help, то выводим справку и возвращаем nullopt
            std::cout << desc;
            return std::nullopt;
        }

        if (!vm.contains("config-file"s)) {
            throw std::runtime_error("Config file path is not specified"s);
        }
        if (!vm.contains("www-root"s)) {
            throw std::runtime_error("Static files root path is not specified"s);
        }

        if (vm.contains("tick-period"s)) {
            args.milliseconds = std::stol(milliseconds);
        }

        args.spawn_points_are_random = vm.contains("randomize-spawn-points"s);

        return args;
    }
    
    void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
        //Выводим время.
        strm << "{\"timestamp\":\"" << to_iso_extended_string(*rec[timestamp]) << "\",";
        //выводим имя ключевого слова содержащего json.
        strm << '\"' << additional_data.get_name() << "\":";
        //Содержание самого json.
        strm << rec[additional_data] << ",";
        // Выводим сообщение.
        strm << "\"message\":\"" << rec[logging::expressions::smessage] << "\"}";
    }

    void InitBoostLogFilter() {
        logging::add_common_attributes();

        logging::add_console_log(
                std::cout,
                keywords::format = &MyFormatter,
                keywords::auto_flush = true
        );
    }
    
}  // namespace

int main(int argc, const char* argv[]) {
    InitBoostLogFilter();

    auto logger = [](json::value&& value, std::string_view message) {
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, value)
                                << message;
    };
    
    /*
    auto logger = [](json::value&& value, std::string_view message) {
        std::cout << json::serialize(value) << " message: " << message << std::endl;
    };
    */
    try {
        auto args = ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_SUCCESS;
        }
        model::Game game;
        extra_data::FrontendData frontend_data;
        {
            // 1. Загружаем конфиг из файла
            json_loader::JsonLoader json_loader(args->file);
            // 1.1 Строим модель игры
            game = json_loader.LoadGame();
            // 1.2 Получаем данные необходимые фронтенду
            frontend_data = json_loader.LoadFrontendData();
        }
        // 1.3 Устанавливаем параметр спавна игроков
        game.SetSpawnPointsRandom(args->spawn_points_are_random);


        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        bool is_update_time_shift_automatic = args->milliseconds.has_value();
        std::shared_ptr<time_shift::Ticker> ticker;

        if (is_update_time_shift_automatic) {
            std::chrono::milliseconds period = args->milliseconds.value() * 1ms;
            ticker = std::make_shared<time_shift::Ticker>(api_strand, period,
                                                               [&game](std::chrono::milliseconds delta) {
                    game.SetTimeShift(std::chrono::duration<double>(delta).count());
                }
            );
            ticker->Start();
        }
        
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(game,
                                                                      args->dir,
                                                                      api_strand,
                                                                      std::move(frontend_data),
                                                                      is_update_time_shift_automatic);

        server_logging::LoggingRequestHandler request_logger{handler};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        http_server::ServeHttp(ioc, {address, port}, [&request_logger, &logger](auto&& endpoint, auto&& req, auto&& send) {
            request_logger(std::forward<decltype(endpoint)>(endpoint),
                           std::forward<decltype(req)>(req),
                           std::forward<decltype(send)>(send),
                           std::forward<decltype(logger)>(logger));
            }, logger);

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        {
            json::value data_for_log_start_server{{"port", port}, {"address", address.to_string()}};
            logger(std::move(data_for_log_start_server), "server started"sv);
        }

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

    } catch (const std::exception& ex) {
        json::value data_for_log_stop_server{{"code", EXIT_FAILURE}, {"exception", ex.what()}};
        logger(std::move(data_for_log_stop_server), "server exited"sv);

        return EXIT_FAILURE;
    }

    json::value data_for_log_stop_server{{"code", 0}};
    logger(std::move(data_for_log_stop_server), "server exited"sv);
}
