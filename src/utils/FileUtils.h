#ifndef GRAPHICSPRAKTIKUM_FILEUTILS_H
#define GRAPHICSPRAKTIKUM_FILEUTILS_H


#include <vector>
#include <fstream>

bool readFile(const std::string& filename, std::vector<char> &result) {
    // TODO: When refactoring this, don't copy return a vector ?
    // Shouldn't be a big problem here though, they are not too large
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    size_t fileSize = (size_t) file.tellg();
    result.resize(fileSize);

    file.seekg(0);
    file.read(result.data(), fileSize);

    file.close();
    return true;
}


#endif //GRAPHICSPRAKTIKUM_FILEUTILS_H
