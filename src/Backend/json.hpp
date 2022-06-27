#ifndef QAT_BACKEND_JSON_HPP
#define QAT_BACKEND_JSON_HPP

#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace qat {
namespace backend {

class JSON;

class JsonValue {
private:
  // Content of this JsonValue
  std::string value;

public:
  // Integer value
  JsonValue(int val);
  // Float value
  JsonValue(float val);
  // Double value
  JsonValue(double val);
  // Boolean value
  JsonValue(bool val);
  // C string value
  JsonValue(const char *val);
  // String value
  JsonValue(std::string val);
  // String without quotes
  explicit JsonValue(std::string val, bool is_raw);
  // A list of values
  JsonValue(std::initializer_list<JsonValue> val);
  // Null value using pointer (for implict constuction)
  JsonValue(std::nullptr_t val);

  // Get the value of this JsonValue
  std::string get() const noexcept;
};

class JSON {
private:
  /**
   * All fields in this JSON object
   */
  std::map<std::string, JsonValue> fields;

  int level;

public:
  JSON();

  JSON &_(std::string key, JsonValue value);
  JSON &_(std::string key, JSON value);

  void setLevel(int _level) noexcept;

  std::string toString() const noexcept;
};

} // namespace backend
} // namespace qat

#endif