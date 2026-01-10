#ifndef QAT_IR_META_INFO_HPP
#define QAT_IR_META_INFO_HPP

#include "../utils/identifier.hpp"
#include "./types/text.hpp"
#include "value.hpp"

#include <helpers/maybe.hpp>
#include <helpers/pair.hpp>
#include <helpers/string.hpp>
#include <helpers/string_view.hpp>
#include <helpers/vec.hpp>

namespace qat::ir {

struct MetaInfo {
	static constexpr StringView foreignKey  = "foreign";
	static constexpr StringView linkAsKey   = "linkAs";
	static constexpr StringView packedKey   = "packed";
	static constexpr StringView inlineKey   = "inline";
	static constexpr StringView providesKey = "provides";

	MetaInfo(Vec<Pair<Identifier, ir::PrerunValue*>> keyValues, Vec<FileRangePtr> _valueRanges, FileRangePtr _fileRange)
	    : valueRanges(_valueRanges), fileRange(_fileRange) {
		for (auto& kv : keyValues) {
			keys.push_back(kv.first);
			values.push_back(kv.second);
		}
	}

	Vec<Identifier>       keys;
	Vec<ir::PrerunValue*> values;
	Vec<FileRangePtr>     valueRanges;
	FileRangePtr          fileRange;

	bool has_key(StringView name) const {
		for (auto& k : keys) {
			if (k.value == name) {
				return true;
			}
		}
		return false;
	}

	ir::PrerunValue* get_value_for(StringView name) const {
		usize ind = 0;
		for (auto& k : keys) {
			if (k.value == name) {
				return values.at(ind);
			}
			ind++;
		}
		return nullptr;
	}

	FileRangePtr get_value_range_for(StringView name) const {
		usize ind = 0;
		for (auto& k : keys) {
			if (k.value == name) {
				return valueRanges.at(ind);
			}
			ind++;
		}
		return fileRange;
	}

	Maybe<String> get_foreign_id() const {
		if (has_key("foreign")) {
			return ir::TextType::value_to_string(get_value_for("foreign"));
		}
		return None;
	}

	Maybe<String> get_value_as_string_for(StringView key) const {
		if (has_key(key)) {
			return ir::TextType::value_to_string(get_value_for(key));
		}
		return None;
	}

	Maybe<bool> get_value_as_bool(StringView key) const {
		if (has_key(key)) {
			return llvm::cast<llvm::ConstantInt>(get_value_for(key)->get_llvm_constant())->getValue().getBoolValue();
		}
		return None;
	}

	// NOTE - Should the return value be optional?
	bool get_inline() const {
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
