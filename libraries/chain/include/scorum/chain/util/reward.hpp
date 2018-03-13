#pragma once

#include <scorum/chain/util/asset.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/config.hpp>
#include <scorum/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace scorum {
namespace chain {
namespace util {

using scorum::protocol::asset;
using scorum::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
    share_type rshares;
    uint16_t reward_weight = 0;
    asset max_scr = asset(0, SCORUM_SYMBOL);
    uint128_t total_reward_shares2;
    asset total_reward_fund_scorum = asset(0, SCORUM_SYMBOL);
    curve_id reward_curve = quadratic;
};

asset get_rshare_reward(const comment_reward_context& ctx);

uint128_t evaluate_reward_curve(const uint128_t& rshares, const curve_id& curve = quadratic);

// SCORUM: decide who will we approach MIN PAYOUT if we don't have stable coin
inline bool is_comment_payout_dust(uint64_t scorum_payout)
{
    // SCORUM: original logic of steem
    // return to_sbd( p, asset( scorum_payout, SCORUM_SYMBOL ) ) < SCORUM_MIN_PAYOUT_SBD;

    // SCORUM: it make sense to move SCORUM_MIN_COMMENT_PAYOUT to global properties and let witnesses vote on it
    return asset(scorum_payout, SCORUM_SYMBOL) < SCORUM_MIN_COMMENT_PAYOUT;

    // SCORUM: no payout limits
    // return scorum_payout <= 0;
}
}
}
} // scorum::chain::util

FC_REFLECT(scorum::chain::util::comment_reward_context,
           (rshares)(reward_weight)(max_scr)(total_reward_shares2)(total_reward_fund_scorum)(reward_curve))
