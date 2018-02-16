#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <scorum/protocol/scorum_operations.hpp>

namespace scorum {
namespace chain {

dbs_witness::dbs_witness(database& db)
    : _base_type(db)
{
}

const witness_object& dbs_witness::get(const account_name_type& name) const
{
    try
    {
        return db_impl().get<witness_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

bool dbs_witness::is_exists(const account_name_type& name) const
{
    return nullptr != db_impl().find<witness_object, by_name>(name);
}

const witness_schedule_object& dbs_witness::create_witness_schedule(const modifier_type& modifier)
{
    return db_impl().create<witness_schedule_object>([&](witness_schedule_object& o) { modifier(o); });
}

const witness_schedule_object& dbs_witness::get_witness_schedule_object() const
{
    try
    {
        return db_impl().get<witness_schedule_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

bool dbs_witness::is_exists() const
{
    return nullptr != db_impl().find<witness_schedule_object>();
}

const witness_object& dbs_witness::get_top_witness() const
{
    const auto& idx = db_impl().get_index<witness_index>().indices().get<by_vote_name>();
    FC_ASSERT(idx.begin() != idx.end(), "Empty witness_index by_vote_name.");
    return (*idx.begin());
}

const witness_object& dbs_witness::create_witness(const account_name_type& owner,
                                                  const std::string& url,
                                                  const public_key_type& block_signing_key,
                                                  const chain_properties& props)
{
    FC_ASSERT(owner.size(), "Witness 'owner_name' should not be empty.");
    FC_ASSERT(block_signing_key != public_key_type(), "Witness 'block_signing_key' should not be empty.");

    const auto& dprops = db_impl().get_dynamic_global_properties();

    const auto& new_witness = create_internal(owner, block_signing_key);

    db_impl().modify(new_witness, [&](witness_object& w) {
        fc::from_string(w.url, url);
        w.created = dprops.time;
        w.proposed_chain_props = props;
    });

    return new_witness;
}

const witness_object& dbs_witness::create_initial_witness(const account_name_type& owner,
                                                          const public_key_type& block_signing_key)
{
    const auto& new_witness = create_internal(owner, block_signing_key);

    db_impl().modify(new_witness, [&](witness_object& w) { w.schedule = witness_object::top20; });

    return new_witness;
}

const witness_object& dbs_witness::create_internal(const account_name_type& owner,
                                                   const public_key_type& block_signing_key)
{
    const auto& new_witness = db_impl().create<witness_object>([&](witness_object& w) {
        w.owner = owner;
        w.signing_key = block_signing_key;
        w.hardfork_time_vote = db_impl().get_genesis_time();
    });

    return new_witness;
}

void dbs_witness::update_witness(const witness_object& witness,
                                 const std::string& url,
                                 const public_key_type& block_signing_key,
                                 const chain_properties& props)
{
    FC_ASSERT(block_signing_key != public_key_type(), "Witness 'block_signing_key' should not be empty.");

    db_impl().modify(witness, [&](witness_object& w) {
        fc::from_string(w.url, url);
        w.signing_key = block_signing_key;
        w.proposed_chain_props = props;
    });
}

void dbs_witness::adjust_witness_votes(const account_object& account, const share_type& delta)
{
    const auto& vidx = db_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = vidx.lower_bound(boost::make_tuple(account.id, witness_id_type()));
    while (itr != vidx.end() && itr->account == account.id)
    {
        adjust_witness_vote(db_impl().get(itr->witness), delta);
        ++itr;
    }
}

void dbs_witness::adjust_witness_vote(const witness_object& witness, const share_type& delta)
{
    const auto& props = db_impl().get_dynamic_global_properties();

    const witness_schedule_object& wso = get_witness_schedule_object();
    db_impl().modify(witness, [&](witness_object& w) {
        auto delta_pos = w.votes.value * (wso.current_virtual_time - w.virtual_last_update);
        w.virtual_position += delta_pos;

        w.virtual_last_update = wso.current_virtual_time;
        w.votes += delta;
        FC_ASSERT(w.votes <= props.total_vesting_shares.amount, "",
                  ("w.votes", w.votes)("props", props.total_vesting_shares));

        w.virtual_scheduled_time
            = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH - w.virtual_position) / (w.votes.value + 1);

        /** witnesses with a low number of votes could overflow the time field and end up with a scheduled time in the
         * past */
        if (w.virtual_scheduled_time < wso.current_virtual_time)
            w.virtual_scheduled_time = fc::uint128::max_value();
    });
}
} // namespace chain
} // namespace scorum
