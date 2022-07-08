add_library(QAT_IR_TYPES
    array.cpp
    core_type.cpp
    float.cpp
    integer.cpp
    reference.cpp
    pointer.cpp
    tuple.cpp
    unsigned.cpp
    void.cpp)

target_link_libraries(QAT_IR_TYPES QAT_UTILS)