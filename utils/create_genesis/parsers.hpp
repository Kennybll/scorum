#pragma once

#include <boost/filesystem.hpp>

#include <string>

namespace scorum {

namespace chain {
class genesis_state_type;
}

namespace util {

using scorum::chain::genesis_state_type;

void load(const std::string&, genesis_state_type&);

struct parser_i
{
    virtual void update(genesis_state_type&) = 0;
};

void save_to_string(genesis_state_type&, std::string&, bool pretty_print = true);

void save_to_file(genesis_state_type&, const std::string& path, bool pretty_print = true);

void print(genesis_state_type&);
}
}
