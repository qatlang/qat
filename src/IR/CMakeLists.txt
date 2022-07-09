add_library(QAT_IR
    block.cpp
    context.cpp
    function.cpp
    global_entity.cpp
    local_value.cpp
    member_function.cpp
    qat_module.cpp
    static_member.cpp
    value.cpp)

add_subdirectory(types/)

target_link_libraries(QAT_IR QAT_LEXER QAT_UTILS QAT_AST QAT_IR_TYPES)