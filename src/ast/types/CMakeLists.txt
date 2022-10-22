add_library(QAT_AST_TYPES
        array.cpp
        cstring.cpp
        float.cpp
        future.cpp
        integer.cpp
        named.cpp
        reference.cpp
        pointer.cpp
        qat_type.cpp
        string_slice.cpp
        template_named_type.cpp
        templated.cpp
        tuple.cpp
        unsigned.cpp
        void.cpp)

target_link_libraries(QAT_AST_TYPES QAT_UTILS)