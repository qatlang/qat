add_library(QAT_IR
        context.cpp
        function.cpp
        global_entity.cpp
        member_function.cpp
        control_flow.cpp
        qat_module.cpp
        static_member.cpp
        use.cpp
        value.cpp)

add_subdirectory(types/)

target_link_libraries(QAT_IR QAT_LEXER QAT_UTILS QAT_AST QAT_IR_TYPES)