#pragma once

#include <scorum/protocol/committee.hpp>
#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/registration_committee.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {

struct committee_factory
{
    committee_factory(data_service_factory_i& factory)
        : _factory(factory)
    {
    }

    template <typename OperationType>
    protocol::committee_service_i& obtain_committee(const OperationType&)
    {
        FC_THROW_EXCEPTION(fc::assert_exception, "Operation not implemented.");
    }

    scorum::chain::data_service_factory_i& _factory;
};

template <>
protocol::committee_service_i&
committee_factory::obtain_committee(const protocol::proposal_operation<protocol::registration_committee_i>&)
{
    return _factory.registration_committee_service();
}

} // namespace chain
} // namespace scorum
