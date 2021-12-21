#include <iostream>

#include "utilities/types.cpp"

int main(int count, my::StringConstantList args)
{
    if (count == 1)
    {
        std::cout << "No command or arguments provided!\n"
                  << "Run \"qat help\" for documentation on the available commands" << std::endl;
        return 0;
    }
    else
    {
        if (args[1] == "run")
        {
            if (count != 3)
            {
                std::cout << "Filepath not provided!\n"
                          << "Do \"qat run path/to/my/file.qat\" to compile your application\n"
                          << "Run \"qat help\" for documentation on the available commands" << std::endl;
                return 0;
            }
            else
            {
                /// TODO: 
            }
        }
    }
}