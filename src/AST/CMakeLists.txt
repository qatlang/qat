add_library(QAT_AST
    argument.cpp
    assignment.cpp
    block.cpp
    box.cpp
    bring_paths.cpp
    bring_entities.cpp
    define_core_type.cpp
    errors.cpp
    expose_box.cpp
    expression.cpp
    expression_sentence.cpp
    function_definition.cpp
    function_prototype.cpp
    give_sentence.cpp
    global_declaration.cpp
    if_else.cpp
    lib.cpp
    local_declaration.cpp
    loop_n_times.cpp
    loop_while.cpp
    multithread.cpp
    multithread_give.cpp
    say_sentence.cpp)

add_subdirectory(types/)
add_subdirectory(expressions/)

target_link_directories(QAT_AST
    PRIVATE types/
    PRIVATE expressions/)

target_link_libraries(QAT_AST
    QAT_AST_TYPES
    QAT_AST_EXPRESSIONS
    QAT_UTILS
    QAT_IR
    QAT_BACKEND)