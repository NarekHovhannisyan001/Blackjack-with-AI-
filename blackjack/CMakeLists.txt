cmake_minimum_required(VERSION 3.16)
project(BlackjackAI CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)
add_link_options(-static)

add_library(blackjack_lib
    src/Card.cpp
    src/Deck.cpp
    src/Hand.cpp
    src/Player.cpp
    src/Dealer.cpp
    src/GameEngine.cpp
    src/ConsoleView.cpp)
target_include_directories(blackjack_lib PUBLIC include)

add_executable(blackjack src/main.cpp)
target_link_libraries(blackjack PRIVATE blackjack_lib)

enable_testing()
foreach(t card deck hand engine)
    add_executable(test_${t} tests/test_main.cpp tests/test_${t}.cpp)
    target_include_directories(test_${t} PRIVATE include tests)
    target_link_libraries(test_${t} PRIVATE blackjack_lib)
    add_test(NAME test_${t} COMMAND test_${t})
endforeach()
