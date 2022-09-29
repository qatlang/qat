#ifndef QAT_JSON_HPP
#define QAT_JSON_HPP

#include "helpers.hpp"
#include "macros.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace qat {

enum class JsonValueType {
  integer,
  decimal,
  string,
  boolean,
  null,
  json,
  none,
  list
};

class Json;

class JsonValue {
private:
  // The heap allocated data of this JsonValue
  void *data;

  // Type of the value, used to allocate and deallocate, and also to identify
  // data type
  JsonValueType type;

  // Private constructor to manually assign type and data. This is used only for
  JsonValue(JsonValueType type, void *data);

  friend class Json;

public:
  JsonValue();

  // int
  JsonValue(int val);
  void operator=(int val);

  // unsigned
  JsonValue(unsigned val);
  void operator=(unsigned val);

  // unsigned long long
  JsonValue(unsigned long long val);
  void operator=(unsigned long long val);

#if PLATFORM_IS_UNIX
  // uint64_t
  JsonValue(uint64_t val);
  void operator=(uint64_t val);
#endif

  // int64_t
  JsonValue(int64_t val);
  void operator=(const int64_t val);

  // double
  JsonValue(double val);
  void operator=(const double val);

  // std::string
  JsonValue(std::string val);
  void operator=(const std::string val);

  // C string
  JsonValue(const char *val);
  void operator=(const char *val);

  // bool
  JsonValue(bool val);
  void operator=(const bool val);

  // Json
  JsonValue(Json const &val);
  void operator=(Json const &val);
  JsonValue(Json &&val);
  void operator=(Json &&val);

  // Vector of JsonValue
  JsonValue(std::vector<JsonValue> const &val);
  void operator=(std::vector<JsonValue> const &val);
  JsonValue(std::vector<JsonValue> &&val);
  void operator=(std::vector<JsonValue> &&val);

  // initializer_list of JsonValue
  JsonValue(std::initializer_list<JsonValue> val);
  JsonValue &operator=(const std::initializer_list<JsonValue> &val);

  // Equality & Inequality operators
  //
  // These are provided so that there is no implicit construction of JsonValue
  // instances during comparisons

  bool operator==(const int val) const;
  bool operator!=(const int val) const;
  bool operator==(const unsigned val) const;
  bool operator!=(const unsigned val) const;
  bool operator==(const unsigned long long val) const;
  bool operator!=(const unsigned long long val) const;

#if PLATFORM_IS_UNIX
  bool operator==(const uint64_t val) const;
  bool operator!=(const uint64_t val) const;
#endif

  bool operator==(const int64_t val) const;
  bool operator!=(const int64_t val) const;
  bool operator==(const float val) const;
  bool operator!=(const float val) const;
  bool operator==(const double val) const;
  bool operator!=(const double val) const;
  bool operator==(const char *val) const;
  bool operator!=(const char *val) const;
  bool operator==(const std::string val) const;
  bool operator!=(const std::string val) const;
  bool operator==(const bool val) const;
  bool operator!=(const bool val) const;
  bool operator==(const Json &val) const;
  bool operator!=(const Json &val) const;
  bool operator==(const std::vector<JsonValue> &val) const;
  bool operator!=(const std::vector<JsonValue> &val) const;
  bool operator==(const std::initializer_list<JsonValue> &val) const;
  bool operator!=(const std::initializer_list<JsonValue> &val) const;

  bool operator==(const JsonValue &other) const;
  bool operator!=(const JsonValue &other) const;

  // Copy semantics
  JsonValue(JsonValue const &other);
  JsonValue &operator=(JsonValue const &other);

  // Move semantics
  JsonValue(JsonValue &&other) noexcept;
  JsonValue &operator=(JsonValue &&other) noexcept;

  // A none value. This will have no representation in the resultant json
  static JsonValue none();

  // Check whether this value has a valid json value in it
  explicit operator bool() const;

  // If `isJson` is true, then the json format of the string value is returned.
  // This means that escape characters will be in json format and not actual
  // escape characters
  std::string toString(const bool isJson) const;

  JsonValueType getType() const;

  bool isInt() const;

  int64_t asInt() const;

  bool isDouble() const;

  double asDouble() const;

  bool isNull() const;

  bool isString() const;

  std::string asString() const;

  bool isBool() const;

  bool asBool() const;

  bool isJson() const;

  Json asJson() const;

  bool isNone() const;

  bool isList() const;

  std::vector<JsonValue> asList() const;

  friend std::ostream &operator<<(std::ostream &os, const JsonValue &val);

  void clear();

  ~JsonValue() noexcept;
};

class Json {
private:
  std::vector<std::string> keys;
  std::vector<JsonValue>   values;

  mutable unsigned level  = 0;
  mutable unsigned spaces = 2;

  void setLevel(unsigned lev) const;

  friend class JsonValue;

public:
  Json();

  static Maybe<Json> parse(std::string val);

  Json(Json const &other);

  Json(Json &&other) noexcept;

  Json &_(const std::string key, const JsonValue val);

  Json &operator=(Json const &other);

  Json &operator=(Json &&other) noexcept;

  void setSpaces(unsigned spc) const;

  std::string toString() const;

  bool has(const std::string key) const;

  JsonValue &operator[](const std::string key);

  bool operator==(const Json &other) const;

  bool operator!=(const Json &other) const;

  std::size_t size() const;

  friend std::ostream &operator<<(std::ostream &os, const Json &dt);

  void clear() noexcept;

  ~Json() noexcept;
};

} // namespace qat

#endif