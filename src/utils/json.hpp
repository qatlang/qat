#ifndef QAT_JSON_HPP
#define QAT_JSON_HPP

#include "helpers.hpp"
#include "macros.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace qat {

enum class JsonValueType { integer, decimal, string, boolean, null, json, none, list };

class Json;

class JsonValue {
private:
  // The heap allocated data of this JsonValue
  void* data;

  // Type of the value, used to allocate and deallocate, and also to identify
  // data type
  JsonValueType type;

  // Private constructor to manually assign type and data. This is used only for
  JsonValue(JsonValueType type, void* data);

  friend class Json;

public:
  JsonValue();

  JsonValue(int val);
  JsonValue& operator=(int val);

  JsonValue(unsigned val);
  JsonValue& operator=(unsigned val);

  JsonValue(long val);
  JsonValue& operator=(long val);

  JsonValue(unsigned long val);
  JsonValue& operator=(unsigned long val);

  JsonValue(long long val);
  JsonValue& operator=(long long val);

  JsonValue(unsigned long long val);
  JsonValue& operator=(unsigned long long val);

  // double
  JsonValue(double val);
  JsonValue& operator=(double val);

  // std::string
  JsonValue(String val);
  JsonValue& operator=(const String& val);

  // C string
  JsonValue(const char* val);
  JsonValue& operator=(const char* val);

  // bool
  JsonValue(bool val);
  JsonValue& operator=(bool val);

  // Json
  JsonValue(Json const& val);
  JsonValue& operator=(Json const& val);
  JsonValue(Json&& val);
  JsonValue& operator=(Json&& val);

  // Vector of JsonValue
  JsonValue(Vec<JsonValue> const& val);
  JsonValue& operator=(std::vector<JsonValue> const& val);
  JsonValue(Vec<JsonValue>&& val);
  JsonValue& operator=(std::vector<JsonValue>&& val);

  // initializer_list of JsonValue
  JsonValue(std::initializer_list<JsonValue> val);
  JsonValue& operator=(const std::initializer_list<JsonValue>& val);

  // Equality & Inequality operators
  //
  // These are provided so that there is no implicit construction of JsonValue
  // instances during comparisons

  bool operator==(int val) const;
  bool operator!=(int val) const;
  bool operator==(unsigned val) const;
  bool operator!=(unsigned val) const;
  bool operator==(unsigned long long val) const;
  bool operator!=(unsigned long long val) const;

#if PlatformIsLinux
  bool operator==(uint64_t val) const;
  bool operator!=(uint64_t val) const;
#endif

  bool operator==(int64_t val) const;
  bool operator!=(int64_t val) const;
  bool operator==(float val) const;
  bool operator!=(float val) const;
  bool operator==(double val) const;
  bool operator!=(double val) const;
  bool operator==(const char* val) const;
  bool operator!=(const char* val) const;
  bool operator==(const std::string& val) const;
  bool operator!=(const std::string& val) const;
  bool operator==(bool val) const;
  bool operator!=(bool val) const;
  bool operator==(const Json& val) const;
  bool operator!=(const Json& val) const;
  bool operator==(const std::vector<JsonValue>& val) const;
  bool operator!=(const std::vector<JsonValue>& val) const;
  bool operator==(const std::initializer_list<JsonValue>& val) const;
  bool operator!=(const std::initializer_list<JsonValue>& val) const;

  bool operator==(const JsonValue& other) const;
  bool operator!=(const JsonValue& other) const;

  // Copy semantics
  JsonValue(JsonValue const& other);
  JsonValue& operator=(JsonValue const& other);

  // Move semantics
  JsonValue(JsonValue&& other) noexcept;
  JsonValue& operator=(JsonValue&& other) noexcept;

  // A none value. This will have no representation in the resultant json
  static JsonValue none();

  // Check whether this value has a valid json value in it
  explicit operator bool() const;

  // If `isJson` is true, then the json format of the string value is returned.
  // This means that escape characters will be in json format and not actual
  // escape characters
  useit std::string toString(bool isJson) const;

  useit JsonValueType getType() const;

  useit bool    isInt() const;
  useit int64_t asInt() const;
  useit bool    isDouble() const;
  useit double  asDouble() const;
  useit bool    isNull() const;
  useit bool    isString() const;
  useit std::string asString() const;
  useit bool        isBool() const;
  useit bool        asBool() const;
  useit bool        isJson() const;
  useit Json        asJson() const;
  useit bool        isNone() const;
  useit bool        isList() const;
  useit std::vector<JsonValue> asList() const;

  friend std::ostream& operator<<(std::ostream& os, const JsonValue& val);

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

  static Maybe<Json> parse(const String& val);

  Json(Json const& other);

  Json(Json&& other) noexcept;

  Json& _(const std::string& key, const JsonValue& val);

  Json& operator=(Json const& other);

  Json& operator=(Json&& other) noexcept;

  void setSpaces(unsigned spc) const;

  std::string toString() const;

  bool has(const std::string& key) const;

  JsonValue& operator[](const std::string& key);

  bool operator==(const Json& other) const;

  bool operator!=(const Json& other) const;

  std::size_t size() const;

  friend std::ostream& operator<<(std::ostream& os, const Json& dt);

  void clear() noexcept;

  ~Json() noexcept;
};

} // namespace qat

#endif