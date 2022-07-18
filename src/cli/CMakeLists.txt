add_library(QAT_CLI error.cpp
    config.cpp
    display.cpp)

target_link_directories(QAT_CLI
    PRIVATE ../utils/)

target_link_libraries(QAT_CLI
    QAT_UTILS)