add_library(QAT_AST
   argument.cpp
   box.cpp
   bring_paths.cpp
   bring_entities.cpp
   convertor.cpp
   constructor.cpp
   define_choice_type.cpp
   define_core_type.cpp
   define_mix_type.cpp
   destructor.cpp
   expression.cpp
   function.cpp
   global_declaration.cpp
   lib.cpp
   member_function.cpp
   mod_info.cpp
   node.cpp
   operator_function.cpp
   sentence.cpp
   type_definition.cpp)

add_subdirectory(types/)
add_subdirectory(expressions/)
add_subdirectory(sentences/)
add_subdirectory(constants/)

target_link_directories(QAT_AST
   PRIVATE types/
   PRIVATE expressions/
   PRIVATE sentences/
   PRIVATE constants/)

target_link_libraries(QAT_AST
   QAT_AST_TYPES
   QAT_AST_EXPRESSIONS
   QAT_AST_SENTENCES
   QAT_AST_CONSTANTS
   QAT_UTILS
   QAT_IR
   QAT_BACKEND)