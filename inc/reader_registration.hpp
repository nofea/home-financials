#pragma once

#include "reader_factory.hpp"

// Macro to simplify static registration of BankReader implementations.
// Usage:
//   REGISTER_BANK_READER("Canara", CanaraBankReader)
// This defines a unique static registrar object that calls
// ReaderFactory::registerReader(...) at static initialization time.
#define __RF_CONCAT_INNER(a, b) a##b
#define __RF_CONCAT(a, b) __RF_CONCAT_INNER(a, b)
#define __RF_UNIQUE_NAME(base) __RF_CONCAT(base, __LINE__)

#define REGISTER_BANK_READER(bank_name, ReaderClass)                                \
    namespace {                                                                       \
        struct __RF_UNIQUE_NAME(_ReaderRegistrar_)                                  \
        {                                                                            \
            __RF_UNIQUE_NAME(_ReaderRegistrar_)()                                     \
            {                                                                        \
                ReaderFactory::registerReader(bank_name, []() {                     \
                    return std::make_unique<ReaderClass>();                          \
                });                                                                  \
            }                                                                        \
        } __RF_UNIQUE_NAME(_reader_registrar_instance_);                              \
    }
