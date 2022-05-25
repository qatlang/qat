#include "./file_placement.hpp"

qat::utils::FilePlacement::FilePlacement(fsexp::path _file,
                                         qat::utils::Position _start,
                                         qat::utils::Position _end)
    : file(_file), start(_start), end(_end) {}

qat::utils::FilePlacement::FilePlacement(qat::utils::FilePlacement first,
                                         qat::utils::FilePlacement second)
    : file(first.file), start(first.start),
      end((first.file == second.file) ? second.end : first.end) {}