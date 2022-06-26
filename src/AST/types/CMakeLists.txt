add_library(QAT_AST_TYPES
    array.cpp
    float.cpp
    integer.cpp
    named.cpp
    reference.cpp
    pointer.cpp
    tuple.cpp
    void.cpp)

target_link_libraries(QAT_AST_TYPES QAT_UTILS)