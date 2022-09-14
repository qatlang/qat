add_library(QAT_AST
        argument.cpp
        box.cpp
        bring_paths.cpp
        bring_entities.cpp
        convertor.cpp
        constructor.cpp
        define_core_type.cpp
        errors.cpp
        expression.cpp
        function.cpp
        global_declaration.cpp
        lib.cpp
        member_function.cpp
        node.cpp
        operator_function.cpp
        sentence.cpp
        type_definition.cpp)

add_subdirectory(types/)
add_subdirectory(expressions/)
add_subdirectory(sentences/)

target_link_directories(QAT_AST
        PRIVATE types/
        PRIVATE expressions/
        PRIVATE sentences/)

target_link_libraries(QAT_AST
        QAT_AST_TYPES
        QAT_AST_EXPRESSIONS
        QAT_AST_SENTENCES
        QAT_UTILS
        QAT_IR
        QAT_BACKEND)