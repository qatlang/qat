#include "./space_possibilities.hpp"

std::vector<std::string> qat::utilities::spacePossibilities(std::string value)
{
    std::vector<std::string> parsedValues = qat::utilities::parseSpacesFromIdentifier(value);
    if (parsedValues.size() > 1)
    {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < parsedValues.size(); i++)
        {
            std::string candidate = "";
            for(std::size_t j = 0; j <= i; j++) {
                candidate += parsedValues.at(j);
                candidate += ':';
            }
            result.push_back(candidate);
        }
        return result;
    }
    else
    {
        return parsedValues;
    }
}