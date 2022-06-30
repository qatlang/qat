#include "./json.hpp"
#include <string>
#include <vector>

namespace qat {
namespace backend {

JsonValue::JsonValue(unsigned long long val) : value(std::to_string(val)) {}

JsonValue::JsonValue(unsigned int val) : value(std::to_string(val)) {}

JsonValue::JsonValue(int val) : value(std::to_string(val)) {}

JsonValue::JsonValue(float val) : value(std::to_string(val)) {}

JsonValue::JsonValue(double val) : value(std::to_string(val)) {}

JsonValue::JsonValue(bool val) : value(val ? "true" : "false") {}

JsonValue::JsonValue(const char *val) : value(std::string("\"")) {
  std::string buff(val);
  for (auto ch : buff) {
    switch (ch) {
    case '\a': {
      value += "\\a";
      break;
    }
    case '\b': {
      value += "\\b";
      break;
    }
    case '\t': {
      value += "\\t";
      break;
    }
    case '\n': {
      value += "\\n";
      break;
    }
    case '\v': {
      value += "\\v";
      break;
    }
    case '\f': {
      value += "\\f";
      break;
    }
    case '\r': {
      value += "\\r";
      break;
    }
    case '\e': {
      value += "\\e";
      break;
    }
    case '\"': {
      value += "\\\"";
      break;
    }
    case '\0': {
      value += "\\0";
      break;
    }
    case '\\': {
      value += "\\\\";
      break;
    }
    case '\?': {
      value += "\\?";
      break;
    }
    default: {
      value += ch;
    }
    }
  }
  value += "\"";
}

JsonValue::JsonValue(std::string val) : value("\"") {
  for (auto ch : val) {
    switch (ch) {
    case '\a': {
      value += "\\a";
      break;
    }
    case '\b': {
      value += "\\b";
      break;
    }
    case '\t': {
      value += "\\t";
      break;
    }
    case '\n': {
      value += "\\n";
      break;
    }
    case '\v': {
      value += "\\v";
      break;
    }
    case '\f': {
      value += "\\f";
      break;
    }
    case '\r': {
      value += "\\r";
      break;
    }
    case '\e': {
      value += "\\e";
      break;
    }
    case '\"': {
      value += "\\\"";
      break;
    }
    case '\0': {
      value += "\\0";
      break;
    }
    case '\\': {
      value += "\\\\";
      break;
    }
    case '\?': {
      value += "\\?";
      break;
    }
    default: {
      value += ch;
    }
    }
  }
  value += "\"";
}

JsonValue::JsonValue(std::initializer_list<JsonValue> val) : value("[") {
  unsigned i = 0;
  for (auto elem : val) {
    value += elem.get();
    if (i != (val.size() - 1)) {
      value += ",";
    }
    i++;
  }
  value += "]";
}

JsonValue::JsonValue(std::vector<JsonValue> val) : value("[") {
  unsigned i = 0;
  for (auto elem : val) {
    value += elem.get();
    if (i != (val.size() - 1)) {
      value += ",";
    }
    i++;
  }
  value += "]";
}

JsonValue::JsonValue(std::string val, bool is_raw) : value(val) {}

JsonValue::JsonValue(std::nullptr_t val) : value("null") {}

JsonValue::JsonValue(utils::FilePlacement val) {
  value =
      JSON()
          ._("file", val.file.string())
          ._("start", JSON()
                          ._("line", val.start.line)
                          ._("character", val.start.character))
          ._("end",
             JSON()._("line", val.end.line)._("character", val.end.character))
          .toString();
}

JsonValue::JsonValue(utils::VisibilityInfo visibility) {
  value = JSON()
              ._("kind", utils::Visibility::get_value(visibility.kind))
              ._("value", visibility.value)
              .toString();
}

std::string JsonValue::get() const { return value; }

JSON::JSON() : fields(), level(0) {}

JSON &JSON::_(std::string key, JsonValue value) {
  fields.insert({key, value});
  return *this;
}

JSON &JSON::_(std::string key, JSON value) {
  value.setLevel(level + 1);
  fields.insert({key, JsonValue(value.toString(), true)});
  return *this;
}

JSON &JSON::_(std::string key, std::vector<JSON> values) {
  std::vector<JsonValue> jvals;
  for (auto val : values) {
    val.setLevel(level + 1);
    jvals.push_back(JsonValue(val.toString(), true));
  }
  fields.insert({key, JsonValue(jvals)});
  return *this;
}

void JSON::setLevel(int _level) { level = _level; }

std::string JSON::toString() const {
  std::string result = "{\n";
  unsigned i = 0;
  fields.size();
  for (auto field : fields) {
    for (int j = 0; j < (level + 1); j++) {
      result += "  ";
    }
    result += ("\"" + field.first + '"');
    result += " : ";
    result += field.second.get();
    if (i != (fields.size() - 1)) {
      result += ",\n";
    } else {
      result += "\n";
    }
    i++;
  }
  for (int j = 0; j < level; j++) {
    result += "  ";
  }
  result += "}";
  return result;
}

} // namespace backend
} // namespace qat