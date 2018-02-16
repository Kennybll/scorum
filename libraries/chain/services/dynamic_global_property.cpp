#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

dbs_dynamic_global_property::dbs_dynamic_global_property(database& db)
    : _base_type(db)
{
}

time_point_sec dbs_dynamic_global_property::get_genesis_time() const
{
    return db_impl().get_genesis_time();
}

const dynamic_global_property_object& dbs_dynamic_global_property::get() const
{
    try
    {
        return db_impl().get<dynamic_global_property_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

bool dbs_dynamic_global_property::is_exists() const
{
    return nullptr != db_impl().find<dynamic_global_property_object>();
}

const dynamic_global_property_object& dbs_dynamic_global_property::create(const modifier_type& modifier)
{
    return db_impl().create<dynamic_global_property_object>([&](dynamic_global_property_object& o) { modifier(o); });
}

void dbs_dynamic_global_property::update(const modifier_type& modifier)
{
    db_impl().modify(get(), [&](dynamic_global_property_object& cvo) { modifier(cvo); });
}

fc::time_point_sec dbs_dynamic_global_property::head_block_time() const
{
    return get().time;
}

void dbs_dynamic_global_property::set_invite_quorum(uint64_t quorum)
{
    const dynamic_global_property_object& properties = get();
    db_impl().modify(properties, [&](dynamic_global_property_object& p) { p.invite_quorum = quorum; });
}

void dbs_dynamic_global_property::set_dropout_quorum(uint64_t quorum)
{
    const dynamic_global_property_object& properties = get();
    db_impl().modify(properties, [&](dynamic_global_property_object& p) { p.dropout_quorum = quorum; });
}

void dbs_dynamic_global_property::set_quorum(uint64_t quorum)
{
    const dynamic_global_property_object& properties = get();
    db_impl().modify(properties, [&](dynamic_global_property_object& p) { p.change_quorum = quorum; });
}

} // namespace scorum
} // namespace chain
