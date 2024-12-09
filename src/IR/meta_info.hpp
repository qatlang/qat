#ifndef QAT_IR_META_INFO_HPP
#define QAT_IR_META_INFO_HPP

#include "../utils/identifier.hpp"
#include "./types/string_slice.hpp"
#include "value.hpp"

namespace qat::ir {

struct MetaInfo {
	static constexpr auto foreignKey  = "foreign";
	static constexpr auto linkAsKey   = "linkAs";
	static constexpr auto unionKey    = "union";
	static constexpr auto packedKey   = "packed";
	static constexpr auto inlineKey   = "inline";
	static constexpr auto providesKey = "provides";

	MetaInfo(Vec<Pair<Identifier, ir::PrerunValue*>> keyValues, Vec<FileRange> _valueRanges, FileRange _fileRange)
	    : valueRanges(_valueRanges), fileRange(_fileRange) {
		for (auto& kv : keyValues) {
			keys.push_back(kv.first);
			values.push_back(kv.second);
		}
	}

	Vec<Identifier>       keys;
	Vec<ir::PrerunValue*> values;
	Vec<FileRange>        valueRanges;
	FileRange             fileRange;

	useit bool has_key(String const& name) const {
		for (auto& k : keys) {
			if (k.value == name) {
				return true;
			}
		}
		return false;
	}
	useit ir::PrerunValue* get_value_for(String const& name) const {
		usize ind = 0;
		for (auto& k : keys) {
			if (k.value == name) {
				return values.at(ind);
			}
			ind++;
		}
		return nullptr;
	}

	useit FileRange get_value_range_for(String const& name) const {
		usize ind = 0;
		for (auto& k : keys) {
			if (k.value == name) {
				return valueRanges.at(ind);
			}
			ind++;
		}
		return fileRange;
	}

	useit Maybe<String> get_foreign_id() const {
		if (has_key("foreign")) {
			return ir::StringSliceType::value_to_string(get_value_for("foreign"));
		}
		return None;
	}

	useit Maybe<String> get_value_as_string_for(String key) const {
		if (has_key(key)) {
			return ir::StringSliceType::value_to_string(get_value_for(key));
		}
		return None;
	}

	useit Maybe<bool> get_value_as_bool(String key) const {
		if (has_key(key)) {
			return llvm::cast<llvm::ConstantInt>(get_value_for(key)->get_llvm_constant())->getValue().getBoolValue();
		}
		return None;
	}

	// NOTE - Should the return value be optional?
	useit bool get_inline() const {
		if (has_key(inlineKey)) {
			return llvm::cast<llvm::ConstantInt>(get_value_for(inlineKey)->get_llvm_constant())
			    ->getValue()
			    .getBoolValue();
		}
		return false;
	}
};

} // namespace qat::ir

#endif
