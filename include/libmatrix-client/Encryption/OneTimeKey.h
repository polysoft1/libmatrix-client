#ifndef LIBMATRIX_ENCRYPT_ONETIME_KEYS_H
#define LIBMATRIX_ENCRYPT_ONETIME_KEYS_H

#include <string>

namespace LibMatrix::Encryption{
class OneTimeKey {
private:
    std::string algo;
    std::string head;
    std::string tail;
public:
    OneTimeKey(std::string algo, std::string head, std::string tail) : algo(algo), head(head), tail(tail) {}

    const std::string& getAlgo() const { return algo; }
    const std::string& getHead() const { return head; }
    const std::string& getTail() const { return tail; }
};
}

#endif