#include <scorum/chain/genesis/initializators/witnesses_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <fc/exception/exception.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void witnesses_initializator_impl::on_apply(initializator_context& ctx)
{
    account_service_i& account_service = ctx.services().account_service();
    witness_service_i& witness_service = ctx.services().witness_service();

    for (auto& witness : ctx.genesis_state().witness_candidates)
    {
        FC_ASSERT(!witness.name.empty(), "Witness 'name' should not be empty.");

        account_service.check_account_existence(witness.name);

        FC_ASSERT(witness.block_signing_key != public_key_type(), "Witness 'block_signing_key' should not be empty.");

        witness_service.create_initial_witness(witness.name, witness.block_signing_key);
    }
}
}
}
}
