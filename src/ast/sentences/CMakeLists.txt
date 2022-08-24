add_library(QAT_AST_SENTENCES
        assignment.cpp
        block.cpp
        break.cpp
        continue.cpp
        expose_box.cpp
        expression_sentence.cpp
        give_sentence.cpp
        if_else.cpp
        local_declaration.cpp
        loop_infinite.cpp
        loop_n_times.cpp
        loop_while.cpp
        multithread_give.cpp
        multithread.cpp
        say_sentence.cpp)

target_link_libraries(QAT_AST_SENTENCES QAT_UTILS)