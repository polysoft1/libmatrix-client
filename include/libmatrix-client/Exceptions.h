#ifndef LIBMATRIX_EXCEPTIONS_H
#define LIBMATRIX_EXCEPTIONS_H

#include <string>
#include <stdexcept>

namespace LibMatrix::Exceptions {
    enum OLMError {ACC_CREATE, ID_KEY_GEN, SIGNATURE};
    class OLMException : public std::runtime_error {
        private:
            OLMError err;
        public:
            OLMException(std::string err_msg, OLMError err) : err(err), std::runtime_error(err_msg) {}
            const OLMError type() const { return err; }
    };
}

#endif //LIBMATRIX_EXCEPTIONS_H