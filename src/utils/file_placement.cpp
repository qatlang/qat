#include "./file_placement.hpp"

qat::utilities::FilePlacement::FilePlacement(fsexp::path _file,
                                             qat::utilities::Position _start,
                                             qat::utilities::Position _end)
    : file(_file), start(_start), end(_end) {}

qat::utilities::FilePlacement::FilePlacement(
    qat::utilities::FilePlacement first, qat::utilities::FilePlacement second)
    : file(first.file), start(first.start),
      end((first.file == second.file) ? second.end : first.end) {}