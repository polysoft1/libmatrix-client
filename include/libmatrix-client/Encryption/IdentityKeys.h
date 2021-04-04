#ifndef LIBMATRIX_ENCRYPT_IDKEYS_H
#define LIBMATRIX_ENCRYPT_IDKEYS_H

#include <string>

namespace LibMatrix::Encryption{
class IdentityKey {
private:
    std::string algo;
    std::string value;
public:
    IdentityKey(std::string algo, std::string value) : algo(algo), value(value) {}

    const std::string& getAlgo() const { return algo; }
    const std::string& getValue() const { return value; }
};
}

#endif