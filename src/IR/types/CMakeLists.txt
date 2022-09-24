add_library(QAT_IR_TYPES
        array.cpp
        choice.cpp
        core_type.cpp
        cstring.cpp
        definition.cpp
        float.cpp
        function.cpp
        integer.cpp
        reference.cpp
        pointer.cpp
        qat_type.cpp
        string_slice.cpp
        tuple.cpp
        mix.cpp
        unsigned.cpp
        void.cpp)

target_link_libraries(QAT_IR_TYPES QAT_UTILS)