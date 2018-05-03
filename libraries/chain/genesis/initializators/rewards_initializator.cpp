#include <scorum/chain/genesis/initializators/rewards_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void rewards_initializator_impl::on_apply(initializator_context& ctx)
{
    FC_ASSERT(ctx.genesis_state().rewards_supply.symbol() == SCORUM_SYMBOL);

    create_scr_reward_fund(ctx);
    create_sp_reward_fund(ctx);
    create_balancers(ctx);
    create_fund_budget(ctx);
}

void rewards_initializator_impl::create_scr_reward_fund(initializator_context& ctx)
{
    content_reward_fund_scr_service_i& reward_fund_service = ctx.services().content_reward_fund_scr_service();
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    FC_ASSERT(!reward_fund_service.is_exists());

    reward_fund_service.create([&](content_reward_fund_scr_object& rfo) {
        rfo.last_update = dgp_service.head_block_time();
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });
}

void rewards_initializator_impl::create_sp_reward_fund(initializator_context& ctx)
{
    content_reward_fund_sp_service_i& reward_fund_service = ctx.services().content_reward_fund_sp_service();
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    FC_ASSERT(!reward_fund_service.is_exists());

    reward_fund_service.create([&](content_reward_fund_sp_object& rfo) {
        rfo.last_update = dgp_service.head_block_time();
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });
}

void rewards_initializator_impl::create_balancers(initializator_context& ctx)
{
    content_reward_scr_service_i& content_reward_service = ctx.services().content_reward_scr_service();
    voters_reward_scr_service_i& voters_reward_scr_service = ctx.services().voters_reward_scr_service();
    voters_reward_sp_service_i& voters_reward_sp_service = ctx.services().voters_reward_sp_service();

    FC_ASSERT(!content_reward_service.is_exists());
    FC_ASSERT(!voters_reward_scr_service.is_exists());
    FC_ASSERT(!voters_reward_sp_service.is_exists());

    content_reward_service.create([&](content_reward_balancer_scr_object&) {});
    voters_reward_scr_service.create([&](voters_reward_balancer_scr_object&) {});
    voters_reward_sp_service.create([&](voters_reward_balancer_sp_object&) {});
}

void rewards_initializator_impl::create_fund_budget(initializator_context& ctx)
{
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();
    budget_service_i& budget_service = ctx.services().budget_service();

    FC_ASSERT(!budget_service.is_fund_budget_exists());

    fc::time_point deadline = dgp_service.get_genesis_time() + fc::days(SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);

    budget_service.create_fund_budget(asset(ctx.genesis_state().rewards_supply.amount, SP_SYMBOL), deadline);
}
}
}
}
