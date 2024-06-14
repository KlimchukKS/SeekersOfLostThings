cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
 	src/game_model_content_type.h
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
 	src/server_logging.h
  src/http_handler_string_constants.h
  src/static_request_parser.cpp
  src/static_request_parser.h
  src/response_maker.cpp
  src/response_maker.h
  src/api_request_parser.cpp
  src/api_request_parser.h
  src/ticker.cpp
  src/ticker.h
  src/loot_generator.cpp
  src/loot_generator.h
  src/extra_data.h
  src/geom.h
  src/collision_detector.cpp
  src/collision_detector.h
)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost Threads::Threads)
