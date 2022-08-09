add_library(QAT_AST_TYPES
        array.cpp
        cstring.cpp
        float.cpp
        integer.cpp
        named.cpp
        reference.cpp
        pointer.cpp
        string_slice.cpp
        tuple.cpp
        unsigned.cpp
        void.cpp)

target_link_libraries(QAT_AST_TYPES QAT_UTILS)