#include "./file_range.hpp"
#include "./qat_region.hpp"

namespace qat {

FilePos::FilePos(u64 _line, u64 _byte) : line(_line), byteOffset(_byte) {}

std::ostream& operator<<(std::ostream& os, FilePos const& pos) { return os << pos.line << ":" << pos.byteOffset; }

FileRangePtr FileRange::null = nullptr;

FileRangePtr FileRange::from_path(String* _filePath) {
	return std::construct_at(OwnNormal(FileRange), std::move(_filePath), FilePos{0u, 0u}, FilePos{0u, 0u});
}

FileRangePtr FileRange::from(String* _file, FilePos _start, FilePos _end) {
	return std::construct_at(OwnNormal(FileRange), std::move(_file), _start, _end);
}

FileRange* FileRange::var_from(String* _file, FilePos _start, FilePos _end) {
	return std::construct_at(OwnNormal(FileRange), std::move(_file), _start, _end);
}

FileRangePtr FileRange::merge(FileRange const* first, FileRange const* second) {
	return std::construct_at(OwnNormal(FileRange), first->file, first->start, second->end);
}

FileRangePtr FileRange::spanTo(FileRangePtr other) const { return FileRange::merge(this, other); }

FileRangePtr FileRange::trimTo(FilePos othStart) const { return FileRange::from(file, start, othStart); }

String FileRange::start_to_string() const {
	return *file + ":" + std::to_string(start.line) + ":" + std::to_string(start.byteOffset);
}

bool FileRange::is_before(FileRangePtr another) const {
	return ((end.line < another->start.line) ||
	        ((end.line == another->start.line) && (end.byteOffset < another->start.byteOffset)));
}

String FileRange::to_string() const { return *file + ":" + start.to_string() + " - " + end.to_string(); }

} // namespace qat
