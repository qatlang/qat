#ifndef QAT_AST_SENTENCES_MULTITHREAD_HPP
#define QAT_AST_SENTENCES_MULTITHREAD_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "../types/qat_type.hpp"
#include "./block.hpp"

#include <optional>
#include <string>

namespace qat {
namespace AST {

/**
 *  Multithread represents a sentence that allows spawning n number of
 * threads and executing code and retrieving results if specified by the
 user.
 *
 */
class Multithread : public Sentence {
private:
  //  The number of threads to spawn. This should be of integer type
  Expression *count;

  // The optional type of the entity to store the results of the
  // execution of the threads
  std::optional<QatType *> type;

  // The optional name of the entity to store the results of the
  // execution of the threads
  std::optional<std::string> name;

  // Main block of the multithread sentence. This is executed successfully in
  // each thread spawned if the number of threads specified is greater than 0.
  Block *block;

  // Block for adding declarations and other sentences that are related
  // to multithreading
  Block *cache_block;

  //  Cache block to add the loop that starts different threads
  Block *call_block;

  // Cache block to add the loop that joins different threads
  Block *join_block;

  // The block after the main block. This is not part of the multithread
  // sentence, except that the result obtained from the threads are
  // recognised to be declared in the scope of this variable
  Block *after;

public:
  /**
   *  Multithread represents a sentence that allows spawning n number
   * of threads and executing code and retrieving results if specified by the
   * user.
   *
   * This creates a Multithread block that doesn't give any results
   *
   * @param _count Expression telling the number
   * @param _main Block present inside each of the threads that will be
   spawned
   * @param _after Block present after the multithread block
   * @param _filePlacement
   */
  Multithread(Expression *_count, Block *_main, Block *_after,
              utils::FilePlacement _filePlacement);

  /**
   *  Multithread represents a sentence that allows spawning n number
   * of threads and executing code and retrieving results if specified by the
   * user.
   *
   * This creates a Multithread block that doesn't give any results
   *
   * @param _count Expression telling the number
   * @param _name Name of the variable to store the results in
   * @param _type Type of the result of execution of a single thread
   * @param _main Block present inside each of the threads that will be
   spawned
   * @param _after Block present after the multithread block
   * @param _filePlacement
   */
  Multithread(Expression *_count, std::string _name, QatType *_type,
              Block *_main, Block *_after, utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::multithread; }
};

} // namespace AST
} // namespace qat

#endif