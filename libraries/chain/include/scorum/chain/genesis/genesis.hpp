#pragma once

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {

class database;

class db_genesis
{
public:
    db_genesis(db_genesis const&) = delete;
    db_genesis& operator=(db_genesis const&) = delete;

    db_genesis(database& db, const genesis_state_type& genesis_state);

private:
    database& _db;
    genesis_state_type _genesis_state;
};

} // namespace chain
} // namespace scorum
