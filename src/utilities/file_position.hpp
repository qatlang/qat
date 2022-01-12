#ifndef QAT_UTILITIES_FILE_POSITION_HPP
#define QAT_UTILITIES_FILE_POSITION_HPP

#include <experimental/filesystem>

namespace fsexp = std::experimental::filesystem;

namespace qat
{
    namespace utilities
    {
        class FilePosition
        {
        public:
            FilePosition(fsexp::path _file, long long _line, long long _character)
                : file(_file), line(_line), character(_character) {}

            fsexp::path file;
            long long line;
            long long character;
        };
    }
}

#endif