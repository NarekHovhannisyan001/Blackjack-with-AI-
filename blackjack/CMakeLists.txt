cmake_minimum_required(VERSION 3.16)
project(BlackjackAI C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)
add_link_options(-static)

# ── SQLite amalgamation (compiled as a C library) ──────────────────────────
add_library(sqlite3_lib STATIC vendor/sqlite/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC vendor/sqlite)
set_target_properties(sqlite3_lib PROPERTIES
    C_STANDARD 99
    COMPILE_FLAGS "-DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=1"
)

# ── Game library ────────────────────────────────────────────────────────────
add_library(blackjack_lib
    src/Card.cpp
    src/Deck.cpp
    src/Hand.cpp
    src/Player.cpp
    src/Dealer.cpp
    src/GameEngine.cpp
    src/ConsoleView.cpp
    src/SqliteWrappers.cpp
    src/GameRecord.cpp
    src/Database.cpp
    # Phase 3 sources
    src/StateEncoder.cpp
    src/PolicyManager.cpp
    src/QLearningAgent.cpp
    src/MonteCarloAgent.cpp
    src/TrainingRunner.cpp
)
target_include_directories(blackjack_lib PUBLIC include)
target_link_libraries(blackjack_lib PUBLIC sqlite3_lib)

if(UNIX)
    target_link_libraries(blackjack_lib PUBLIC pthread dl)
endif()

# ── Main executable ─────────────────────────────────────────────────────────
add_executable(blackjack src/main.cpp)
target_link_libraries(blackjack PRIVATE blackjack_lib)

# ── Training executable ──────────────────────────────────────────────────────
add_executable(blackjack_train src/train_main.cpp)
target_link_libraries(blackjack_train PRIVATE blackjack_lib)

# ── Tests ───────────────────────────────────────────────────────────────────
enable_testing()
# Phase 1+2 tests
foreach(t card deck hand engine database)
    add_executable(test_${t} tests/test_main.cpp tests/test_${t}.cpp)
    target_include_directories(test_${t} PRIVATE include tests vendor/sqlite)
    target_link_libraries(test_${t} PRIVATE blackjack_lib)
    add_test(NAME test_${t} COMMAND test_${t})
endforeach()

# Phase 3 tests
foreach(t state_encoder qlearning montecarlo training)
    add_executable(test_${t} tests/test_main.cpp tests/test_${t}.cpp)
    target_include_directories(test_${t} PRIVATE include tests vendor/sqlite)
    target_link_libraries(test_${t} PRIVATE blackjack_lib)
    add_test(NAME test_${t} COMMAND test_${t})
endforeach()

# Release-mode optimisation for training
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(blackjack_lib PRIVATE -O3 -march=native)
endif()
