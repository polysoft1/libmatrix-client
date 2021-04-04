#ifndef LIBMATRIX_ENCRYPT_ACCOUNT_H
#define LIBMATRIX_ENCRYPT_ACCOUNT_H

#include "Encryption/IdentityKeys.h"

#include "olm/olm.h"
#include <vector>

namespace LibMatrix::Encryption {
class Account {
private:
    OlmAccount *account;
    std::vector<IdentityKey> idKeys;

public:
    Account();
    ~Account();
    void clear();

    void generateIdKeys();
    const std::vector<IdentityKey>& getIdKeys() const { return idKeys; }

    std::string sign(std::string message);
};
}

#endif