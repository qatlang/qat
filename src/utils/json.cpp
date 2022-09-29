#include "./json.hpp"
#include "./json_parser.hpp"
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <vector>

namespace qat {

JsonValue::JsonValue(JsonValueType type, void *data) : data(data), type(type) {}

JsonValue JsonValue::none() { return {JsonValueType::none, nullptr}; }

JsonValue::operator bool() const { return (type != JsonValueType::none); }

JsonValue::JsonValue() : data(nullptr), type(JsonValueType::null) {}

JsonValue::JsonValue(const int val)
    : data(new int64_t(val)), type(JsonValueType::integer) {}

void JsonValue::operator=(int val) {
  if (isInt()) {
    *((int64_t *)data) = (int64_t)val;
  } else {
    clear();
    type = JsonValueType::integer;
    data = new int64_t(val);
  }
}

JsonValue::JsonValue(unsigned val)
    : data(new int64_t((int64_t)val)), type(JsonValueType::integer) {}

void JsonValue::operator=(unsigned val) {
  if (isInt()) {
    *((int64_t *)data) = (int64_t)val;
  } else {
    clear();
    type = JsonValueType::integer;
    data = new int64_t((int64_t)val);
  }
}

JsonValue::JsonValue(unsigned long long val)
    : data(new int64_t((int64_t)val)), type(JsonValueType::integer) {}

void JsonValue::operator=(unsigned long long val) {
  if (isInt()) {
    *((int64_t *)data) = (int64_t)val;
  } else {
    clear();
    type = JsonValueType::integer;
    data = new int64_t((int64_t)val);
  }
}

#if PLATFORM_IS_UNIX
JsonValue::JsonValue(const uint64_t val)
    : data(new int64_t((int64_t)val)), type(JsonValueType::integer) {}

void JsonValue::operator=(const uint64_t val) {
  if (isInt()) {
    *((int64_t *)data) = (int64_t)val;
  } else {
    clear();
    type = JsonValueType::integer;
    data = new int64_t((int64_t)val);
  }
}
#endif

JsonValue::JsonValue(const int64_t val)
    : data(new int64_t(val)), type(JsonValueType::integer) {}

void JsonValue::operator=(const int64_t val) {
  if (isInt()) {
    *((int64_t *)data) = val;
  } else {
    clear();
    type = JsonValueType::integer;
    data = new int64_t(val);
  }
}

JsonValue::JsonValue(const double val)
    : data(new double(val)), type(JsonValueType::decimal) {}

void JsonValue::operator=(const double val) {
  if (isDouble()) {
    *((double *)data) = val;
  } else {
    clear();
    type = JsonValueType::decimal;
    data = new double(val);
  }
}

JsonValue::JsonValue(const std::string val)
    : data(new std::string(val)), type(JsonValueType::string) {}

void JsonValue::operator=(const std::string val) {
  if (isString()) {
    *((std::string *)data) = val;
  } else {
    clear();
    type = JsonValueType::string;
    data = new std::string(val);
  }
}

JsonValue::JsonValue(const char *val)
    : data(new std::string(val)), type(JsonValueType::string) {}

void JsonValue::operator=(const char *val) {
  if (isString()) {
    *((std::string *)data) = std::string(val);
  } else {
    clear();
    type = JsonValueType::string;
    data = new std::string(val);
  }
}

JsonValue::JsonValue(const bool val)
    : data(new bool(val)), type(JsonValueType::boolean) {}

void JsonValue::operator=(const bool val) {
  if (isBool()) {
    *((bool *)data) = val;
  } else {
    clear();
    type = JsonValueType::boolean;
    data = new bool(val);
  }
}

JsonValue::JsonValue(Json const &val)
    : data(new Json(val)), type(JsonValueType::json) {}

void JsonValue::operator=(Json const &val) {
  if (isJson()) {
    *((Json *)data) = val;
  } else {
    clear();
    type = JsonValueType::json;
    data = new Json(val);
  }
}

JsonValue::JsonValue(Json &&val) : data(new Json()), type(JsonValueType::json) {
  *((Json *)data) = std::move(val);
}

JsonValue::JsonValue(const Vec<JsonValue> &val)
    : data(new Vec<JsonValue>()), type(JsonValueType::list) {
  *((Vec<JsonValue> *)data) = val;
}

void JsonValue::operator=(Vec<JsonValue> const &val) {
  if (isList()) {
    ((Vec<JsonValue> *)data)->clear();
    *((Vec<JsonValue> *)data) = val;
  } else {
    clear();
    type = JsonValueType::list;
    data = new Vec<JsonValue>(val);
  }
}

JsonValue::JsonValue(Vec<JsonValue> &&val)
    : data(new Vec<JsonValue>()), type(JsonValueType::list) {
  *((Vec<JsonValue> *)data) = std::move(val);
}

void JsonValue::operator=(Vec<JsonValue> &&val) {
  if (isList()) {
    ((Vec<JsonValue> *)data)->clear();
    *((Vec<JsonValue> *)data) = std::move(val);
  } else {
    clear();
    type = JsonValueType::list;
    data = new Vec<JsonValue>(std::move(val));
  }
}

JsonValue::JsonValue(const std::initializer_list<JsonValue> val)
    : data(new Vec<JsonValue>()), type(JsonValueType::list) {
  for (auto const &elem : val) {
    ((Vec<JsonValue> *)data)->push_back(elem);
  }
}

JsonValue &JsonValue::operator=(const std::initializer_list<JsonValue> &val) {
  clear();
  type = JsonValueType::list;
  data = new Vec<JsonValue>();
  for (auto const &elem : val) {
    ((Vec<JsonValue> *)data)->push_back(elem);
  }
  return *this;
}

JsonValue::JsonValue(JsonValue &&other) noexcept {
  type       = other.type;
  data       = other.data;
  other.data = nullptr;
  other.type = JsonValueType::none;
}

JsonValue &JsonValue::operator=(JsonValue &&other) noexcept {
  clear();
  type       = other.type;
  data       = other.data;
  other.data = nullptr;
  other.type = JsonValueType::none;
  return *this;
}

JsonValue::JsonValue(JsonValue const &other)
    : data(nullptr), type(JsonValueType::none) {
  switch (other.type) {
  case JsonValueType::integer: {
    data = new int64_t(*((int64_t *)other.data));
    break;
  }
  case JsonValueType::decimal: {
    data = new double(*((double *)other.data));
    break;
  }
  case JsonValueType::string: {
    data = new std::string(*((std::string *)other.data));
    break;
  }
  case JsonValueType::boolean: {
    data = new bool(*((bool *)other.data));
    break;
  }
  case JsonValueType::json: {
    data = new Json(*((Json *)other.data));
    break;
  }
  case JsonValueType::list: {
    data                    = new Vec<JsonValue>();
    *(Vec<JsonValue> *)data = *((Vec<JsonValue> *)other.data);
    break;
  }
  case JsonValueType::null:
  case JsonValueType::none: {
    break;
  }
  }
  type = other.type;
}

JsonValue &JsonValue::operator=(JsonValue const &other) {
  clear();
  type = other.type;
  switch (type) {
  case JsonValueType::integer: {
    data = new int64_t(*((int64_t *)other.data));
    break;
  }
  case JsonValueType::decimal: {
    data = new double(*((double *)other.data));
    break;
  }
  case JsonValueType::string: {
    data = new std::string(*((std::string *)other.data));
    break;
  }
  case JsonValueType::boolean: {
    data = new bool(*((bool *)other.data));
    break;
  }
  case JsonValueType::json: {
    data = new Json(*((Json *)other.data));
    break;
  }
  case JsonValueType::list: {
    data                    = new Vec<JsonValue>();
    *(Vec<JsonValue> *)data = *((Vec<JsonValue> *)other.data);
    break;
  }
  case JsonValueType::null:
  case JsonValueType::none: {
    break;
  }
  }
  return *this;
}

bool JsonValue::operator==(JsonValue const &other) const {
  if (type != other.type) {
    return false;
  }
  switch (type) {
  case JsonValueType::integer: {
    return (*((int64_t *)data)) == (*((int64_t *)other.data));
  }
  case JsonValueType::decimal: {
    return (*((double *)data)) == (*((double *)other.data));
  }
  case JsonValueType::string: {
    return (*((std::string *)data)) == (*((std::string *)other.data));
  }
  case JsonValueType::boolean: {
    return (*((bool *)data)) == (*((bool *)other.data));
  }
  case JsonValueType::json: {
    return (*((Json *)data)) == (*((Json *)other.data));
  }
  case JsonValueType::list: {
    if (((Vec<JsonValue> *)other.data)->size() !=
        ((Vec<JsonValue> *)data)->size()) {
      return false;
    }
    for (std::size_t i = 0; i < ((Vec<JsonValue> *)data)->size(); i++) {
      if (((Vec<JsonValue> *)data)->at(i) !=
          ((Vec<JsonValue> *)other.data)->at(i)) {
        return false;
      }
    }
    return true;
  }
  case JsonValueType::null:
  case JsonValueType::none: {
    return true;
  }
  }
}

bool JsonValue::operator!=(const JsonValue &other) const {
  return !(*this == other);
}

bool JsonValue::operator==(const int val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) == ((int64_t)val));
  }
  return false;
}
bool JsonValue::operator!=(const int val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) != ((int64_t)val));
  }
  return true;
}

bool JsonValue::operator==(const unsigned val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) == ((int64_t)val));
  }
  return false;
}
bool JsonValue::operator!=(const unsigned val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) != ((int64_t)val));
  }
  return true;
}

bool JsonValue::operator==(const unsigned long long val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) == ((int64_t)val));
  }
  return false;
}
bool JsonValue::operator!=(const unsigned long long val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) != ((int64_t)val));
  }
  return true;
}

#if PLATFORM_IS_UNIX
bool JsonValue::operator==(const uint64_t val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) == ((int64_t)val));
  }
  return false;
}
bool JsonValue::operator!=(const uint64_t val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) != ((int64_t)val));
  }
  return true;
}
#endif

bool JsonValue::operator==(const int64_t val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) == val);
  }
  return false;
}
bool JsonValue::operator!=(const int64_t val) const {
  if (isInt()) {
    return ((*((int64_t *)data)) != val);
  }
  return true;
}

bool JsonValue::operator==(const float val) const {
  if (isDouble()) {
    return ((*((double *)data)) == ((double)val));
  }
  return false;
}
bool JsonValue::operator!=(const float val) const {
  if (isDouble()) {
    return ((*((double *)data)) != ((double)val));
  }
  return true;
}

bool JsonValue::operator==(const double val) const {
  if (isDouble()) {
    return ((*((double *)data)) == val);
  }
  return false;
}
bool JsonValue::operator!=(const double val) const {
  if (isDouble()) {
    return ((*((double *)data)) != val);
  }
  return true;
}

bool JsonValue::operator==(const char *val) const {
  if (isString()) {
    return ((*((std::string *)data)) == std::string(val));
  }
  return false;
}
bool JsonValue::operator!=(const char *val) const {
  if (isString()) {
    return ((*((std::string *)data)) != std::string(val));
  }
  return true;
}

bool JsonValue::operator==(const std::string val) const {
  if (isString()) {
    return ((*((std::string *)data)) == val);
  }
  return false;
}
bool JsonValue::operator!=(const std::string val) const {
  if (isString()) {
    return ((*((std::string *)data)) != val);
  }
  return true;
}

bool JsonValue::operator==(const bool val) const {
  if (isBool()) {
    return ((*((bool *)data)) == val);
  }
  return false;
}
bool JsonValue::operator!=(const bool val) const {
  if (isBool()) {
    return ((*((bool *)data)) != val);
  }
  return true;
}

bool JsonValue::operator==(const Json &val) const {
  if (isJson()) {
    return ((*((Json *)data)) == val);
  }
  return false;
}
bool JsonValue::operator!=(const Json &val) const {
  if (isJson()) {
    return ((*((Json *)data)) != val);
  }
  return true;
}

bool JsonValue::operator==(const Vec<JsonValue> &val) const {
  if (isList()) {
    auto thisList = (Vec<JsonValue> *)data;
    if (thisList->size() == val.size()) {
      for (std::size_t i = 0; i < val.size(); i++) {
        if (thisList->at(i) != val.at(i)) {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  }
  return false;
}
bool JsonValue::operator!=(const Vec<JsonValue> &val) const {
  if (isList()) {
    auto thisList = (Vec<JsonValue> *)data;
    if (thisList->size() == val.size()) {
      for (std::size_t i = 0; i < val.size(); i++) {
        if (thisList->at(i) != val.at(i)) {
          return true;
        }
      }
      return false;
    } else {
      return true;
    }
  }
  return true;
}

bool JsonValue::operator==(const std::initializer_list<JsonValue> &val) const {
  if (isList()) {
    auto thisList = (Vec<JsonValue> *)data;
    if (thisList->size() == val.size()) {
      std::size_t i = 0;
      for (const auto &elem : val) {
        if (thisList->at(i) != elem) {
          return false;
        }
        i++;
      }
      return true;
    } else {
      return false;
    }
  }
  return false;
}
bool JsonValue::operator!=(const std::initializer_list<JsonValue> &val) const {
  if (isList()) {
    auto thisList = (Vec<JsonValue> *)data;
    if (thisList->size() == val.size()) {
      std::size_t i = 0;
      for (const auto &elem : val) {
        if (thisList->at(i) != elem) {
          return true;
        }
        i++;
      }
      return false;
    } else {
      return true;
    }
  }
  return true;
}

std::string JsonValue::toString(const bool isJson) const {
  switch (type) {
  case JsonValueType::string: {
    auto thisStr = (std::string *)data;
    if (isJson) {
      std::string formatted;
      for (auto ch : *thisStr) {
        if (ch == '\n') {
          formatted += "\\n";
        } else if (ch == '\t') {
          formatted += "\\t";
        } else if (ch == '\b') {
          formatted += "\\b";
        } else if (ch == '\f') {
          formatted += "\\f";
        } else if (ch == '\\') {
          formatted += "\\\\";
        } else if (ch == '"') {
          formatted += "\\\"";
        } else {
          formatted += ch;
        }
      }
      return '"' + formatted + '"';
    } else {
      return *thisStr;
    }
  }
  case JsonValueType::integer: {
    return std::to_string(*((int64_t *)data));
  }
  case JsonValueType::decimal: {
    return std::to_string(*((double *)data));
  }
  case JsonValueType::boolean: {
    return (*((bool *)data)) ? "true" : "false";
  }
  case JsonValueType::json: {
    return ((Json *)data)->toString();
  }
  case JsonValueType::list: {
    std::string result("[");
    auto        list = (Vec<JsonValue> *)data;
    for (std::size_t i = 0; i < list->size(); i++) {
      result += list->at(i).toString(isJson);
      if (i != (list->size() - 1)) {
        result += ", ";
      }
    }
    result += "]";
    return result;
  }
  case JsonValueType::null: {
    return "null";
  }
  case JsonValueType::none: {
    return "";
  }
  }
  return "";
}

JsonValueType JsonValue::getType() const { return type; }

bool JsonValue::isBool() const { return (type == JsonValueType::boolean); }

bool JsonValue::asBool() const { return *((bool *)data); }

bool JsonValue::isDouble() const { return (type == JsonValueType::decimal); }

double JsonValue::asDouble() const { return *((double *)data); }

bool JsonValue::isInt() const { return (type == JsonValueType::integer); }

int64_t JsonValue::asInt() const { return *((int64_t *)data); }

bool JsonValue::isJson() const { return (type == JsonValueType::json); }

Json JsonValue::asJson() const { return *((Json *)data); }

bool JsonValue::isList() const { return (type == JsonValueType::list); }

Vec<JsonValue> JsonValue::asList() const { return *((Vec<JsonValue> *)data); }

bool JsonValue::isNull() const { return (type == JsonValueType::null); }

bool JsonValue::isNone() const { return (type == JsonValueType::none); }

bool JsonValue::isString() const { return (type == JsonValueType::string); }

std::string JsonValue::asString() const { return *((std::string *)data); }

std::ostream &operator<<(std::ostream &stream, const JsonValue &val) {
  std::operator<<(stream, val.toString(true));
  return stream;
}

void JsonValue::clear() {
  if (data != nullptr) {
    switch (type) {
    case JsonValueType::string: {
      delete (std::string *)data;
      break;
    }
    case JsonValueType::integer: {
      delete (int64_t *)data;
      break;
    }
    case JsonValueType::decimal: {
      delete (double *)data;
      break;
    }
    case JsonValueType::boolean: {
      delete (bool *)data;
      break;
    }
    case JsonValueType::json: {
      delete (Json *)data;
      break;
    }
    case JsonValueType::list: {
      delete (Vec<JsonValue> *)data;
      break;
    }
    case JsonValueType::null:
    case JsonValueType::none:
      break;
    }
    data = nullptr;
  }
}

JsonValue::~JsonValue() noexcept { clear(); }

Json::Json() = default;

Maybe<Json> Json::parse(std::string val) {
  auto parser = JsonParser();
  if (parser.lex(val)) {
    return parser.parse();
  } else {
    return None;
  }
}

Json::Json(Json const &other) : keys(), values() {
  keys   = other.keys;
  values = other.values;
  level  = other.level;
  spaces = other.spaces;
}

Json::Json(Json &&other) noexcept : keys(), values() {
  keys   = std::move(other.keys);
  values = std::move(other.values);
  other.clear();
  level  = other.level;
  spaces = other.spaces;
}

Json &Json::_(std::string key, JsonValue val) {
  if (has(key)) {
    (*this)[key] = val;
  } else {
    keys.push_back(key);
    values.push_back(val);
  }
  return *this;
}

Json &Json::operator=(Json const &other) {
  clear();
  keys   = other.keys;
  values = other.values;
  level  = other.level;
  spaces = other.spaces;
  return *this;
}

Json &Json::operator=(Json &&other) noexcept {
  clear();
  keys   = std::move(other.keys);
  values = std::move(other.values);
  other.clear();
  level  = other.level;
  spaces = other.spaces;
  return *this;
}

void Json::setLevel(unsigned lev) const {
  level = lev;
  for (auto val : values) {
    if (val.isJson()) {
      ((Json *)val.data)->setLevel(lev + 1);
    }
  }
}

void Json::setSpaces(unsigned spc) const {
  spaces = spc;
  for (auto val : values) {
    if (val.isJson()) {
      ((Json *)val.data)->setSpaces(spc);
    }
  }
}

std::string Json::toString() const {
  if (size() == 0) {
    return "{}";
  } else {
    std::string result("{\n");
    for (std::size_t i = 0; i < keys.size(); i++) {
      if (values.at(i).isNone()) {
        if ((i != (keys.size() - 1)) && (values.at(i + 1))) {
          result += ",\n";
        }
        continue;
      }
      for (std::size_t j = 0; j < (level + 1); j++) {
        for (std::size_t k = 0; k < spaces; k++) {
          result += " ";
        }
      }
      if (values.at(i).isJson()) {
        ((Json *)(values.at(i).data))->setLevel(level + 1);
      }
      result += ('"' + keys.at(i) + '"' + " : " + values.at(i).toString(true));
      if ((i != (keys.size() - 1)) && (values.at(i + 1))) {
        result += ",\n";
      }
    }
    result += "\n";
    for (std::size_t j = 0; j < level; j++) {
      for (std::size_t k = 0; k < spaces; k++) {
        result += " ";
      }
    }
    result += "}";
    return result;
  }
}

bool Json::has(const std::string key) const {
  for (std::size_t i = 0; i < keys.size(); i++) {
    if (keys.at(i) == key) {
      return true;
    }
  }
  return false;
}

JsonValue &Json::operator[](const std::string key) {
  for (std::size_t i = 0; i < keys.size(); i++) {
    if (keys.at(i) == key) {
      return values[i];
    }
  }
  keys.push_back(key);
  auto none = JsonValue::none();
  values.push_back(none);
  return values[values.size() - 1];
}

bool Json::operator==(const Json &other) const {
  if (size() != other.size()) {
    return false;
  }
  for (std::size_t i = 0; i < keys.size(); i++) {
    if (!values.at(i).isNone()) {
      if (keys.at(i) != other.keys.at(i)) {
        return false;
      }
      if (values.at(i) != other.values.at(i)) {
        return false;
      }
    }
  }
  return true;
}

bool Json::operator!=(const Json &other) const { return !((*this) == other); }

std::size_t Json::size() const {
  std::size_t result = 0;
  for (std::size_t i = 0; i < values.size(); i++) {
    if (values.at(i).type != JsonValueType::none) {
      result++;
    }
  }
  return result;
}

std::ostream &operator<<(std::ostream &os, const Json &json) {
  return (os << json.toString() << "\n");
}

void Json::clear() noexcept {
  keys.clear();
  values.clear();
}

Json::~Json() noexcept { clear(); }

} // namespace qat