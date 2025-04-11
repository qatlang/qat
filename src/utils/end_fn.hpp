#ifndef QAT_UTILS_END_FUNCTION_HPP
#define QAT_UTILS_END_FUNCTION_HPP

#include <functional>

class EndFunction {
	std::function<void()> fn;

  public:
	EndFunction(std::function<void()> _fn) : fn(std::move(_fn)) {}

	~EndFunction() { fn(); }
};

#endif
