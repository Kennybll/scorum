#include <scorum/chain/evaluators/proposal_evaluators.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/evaluators/withdraw_scorumpower_evaluator.hpp>

namespace scorum {
namespace chain {

development_committee_transfer_evaluator::development_committee_transfer_evaluator(data_service_factory_i& r)
    : proposal_operation_evaluator<development_committee_transfer_evaluator>(r)
{
}

void development_committee_transfer_evaluator::do_apply(
    const development_committee_transfer_evaluator::operation_type& o)
{
    auto& account_service = this->db().account_service();
    auto& dev_pool = this->db().dev_pool_service();
    auto& dyn_props_service = this->db().dynamic_global_property_service();

    FC_ASSERT(o.amount <= dev_pool.get_scr_balace(), "Not enough SCR in dev pool.");

    dev_pool.decrease_scr_balance(o.amount);

    const auto& account = account_service.get_account(o.to_account);

    account_service.increase_balance(account, o.amount);

    dyn_props_service.update([&](dynamic_global_property_object& dpo) { dpo.circulating_capital += o.amount; });
}

development_committee_withdraw_vesting_evaluator::development_committee_withdraw_vesting_evaluator(
    data_service_factory_i& r)
    : proposal_operation_evaluator<development_committee_withdraw_vesting_evaluator>(r)
{
}

void development_committee_withdraw_vesting_evaluator::do_apply(
    const development_committee_withdraw_vesting_evaluator::operation_type& o)
{
    withdraw_scorumpower_dev_pool_task create_withdraw;
    withdraw_scorumpower_context ctx(db(), o.vesting_shares);
    create_withdraw.apply(ctx);
}

template <budget_type type>
void development_committee_change_top_budgets_amount_evaluator<type>::do_apply(
    const development_committee_change_top_budgets_amount_evaluator::operation_type& o)
{
    auto& dev_pool = this->db().dev_pool_service();

    dev_pool.update([&](dev_committee_object& com) { com.top_budgets_amounts.at(type) = o.amount; });
}

template class development_committee_change_top_budgets_amount_evaluator<budget_type::post>;
template class development_committee_change_top_budgets_amount_evaluator<budget_type::banner>;

} // namespace chain
} // namespace scorum
