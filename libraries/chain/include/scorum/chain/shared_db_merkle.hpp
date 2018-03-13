#pragma once

#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

inline static const std::map<uint32_t, protocol::checksum_type>& get_shared_db_merkle()
{
    static const std::map<uint32_t, protocol::checksum_type> shared_db_merkle{
        { 3705111, protocol::checksum_type("0a8f0fd5450c3706ec8b8cbad795cd0b3679bf35") },
        { 3705120, protocol::checksum_type("2027edb72b671f7011c8cc4c7a8b59c39b305093") },
        { 3713940, protocol::checksum_type("bf8a1d516927c506ebdbb7b38bef2e992435435f") },
        { 3714132, protocol::checksum_type("e8b77773d268b72c8d650337b8cce360bbe64779") },
        { 3714567, protocol::checksum_type("45af59a8c2d7d4a606151ef5dae03d2dfe13fbdd") },
        { 3714588, protocol::checksum_type("e64275443bdc82f104ac936486d367af8f6d1584") },
        { 4138790, protocol::checksum_type("f65a3a788a2ef52406d8ba5705d7288be228403f") },
        { 5435426, protocol::checksum_type("0b32538b2d22bd3146d54b6e3cb5ae8b9780e8a5") }
    };

    return shared_db_merkle;
}
}
} // scorum::chain
