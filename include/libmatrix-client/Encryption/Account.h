#ifndef LIBMATRIX_ENCRYPT_ACCOUNT_H
#define LIBMATRIX_ENCRYPT_ACCOUNT_H

#include "Encryption/IdentityKeys.h"
#include "Encryption/OneTimeKey.h"

#include "olm/olm.h"
#include <vector>
#include <queue>

namespace LibMatrix::Encryption {
class Account {
private:
    OlmAccount *account;
    std::vector<IdentityKey> idKeys;
    std::queue<OneTimeKey> oneTimeKeys;

public:
    Account();
    ~Account();
    void clear();

    void generateIdKeys();
    const std::vector<IdentityKey>& getIdKeys() const { return idKeys; }

    void generateOneTimeKeys(int num = 5);

    std::string sign(std::string message);
};
}

#endif