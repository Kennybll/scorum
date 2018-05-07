#include <boost/test/unit_test.hpp>

#include "blogging_common.hpp"

#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/chain/schema/budget_object.hpp>

#include <scorum/protocol/config.hpp>
#include <boost/make_unique.hpp>

#include <scorum/rewards_math/formulas.hpp>

#define PRINT_CURVE_POINTS

namespace find_asymptote_tests {

using namespace scorum::chain;
using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;
using fc::uint128_t;

using namespace database_fixture;

#ifdef PRINT_CURVE_POINTS
struct voting_power_info
{
    voting_power_info(const percent_type& voting_power_, const fc::time_point_sec& time_, uint32_t sequence_)
        : voting_power(voting_power_)
        , time(time_)
        , sequence(sequence_)
    {
    }

    percent_type voting_power = 0u;
    fc::time_point_sec time;
    uint32_t sequence = 0;
};

struct payout_info
{
    payout_info(const share_type& reward_fund_,
                const fc::uint128_t& total_claims_,
                const share_type& payout_,
                const fc::time_point_sec& payout_time_,
                uint32_t sequence_)
        : reward_fund(reward_fund_)
        , total_claims(total_claims_)
        , payout(payout_)
        , payout_time(payout_time_)
        , sequence(sequence_)
    {
    }

    share_type reward_fund;
    fc::uint128_t total_claims = 0u;
    share_type payout;
    fc::time_point_sec payout_time;
    uint32_t sequence = 0;
};
#endif

struct find_asymptote_fixture : public blogging_common_fixture
{
    find_asymptote_fixture()
        : budget_service(db.budget_service())
        , reward_fund_sp_service(db.reward_fund_sp_service())
        , alice("alice")
        , bob("bob")
        , sam("sam")
        , simon("simon")
    {
        scorum::protocol::detail::override_config(boost::make_unique<scorum::protocol::detail::config>());

        genesis_state_type genesis;

        genesis = Genesis::create()
                      .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY)
                      .rewards_supply(ASSET_SCR(4800000e+9))
                      .witnesses(initdelegate)
                      .accounts(alice, bob, sam, simon)
                      .dev_committee(initdelegate)
                      .generate(fc::time_point_sec::from_iso_string("2018-03-23T14:15:00"));

        open_database(genesis);

        const auto feed_amount = share_value_type(5000e+9);

        actor(initdelegate).give_scr(alice, feed_amount); // 5000
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).give_scr(bob, feed_amount / 10); // 500
        actor(initdelegate).give_sp(bob, feed_amount / 10);

        actor(initdelegate).give_scr(sam, feed_amount / 100); // 50
        actor(initdelegate).give_sp(sam, feed_amount / 100);

        actor(initdelegate).give_scr(simon, feed_amount / 1000); // 5
        actor(initdelegate).give_sp(simon, feed_amount / 1000);

        generate_blocks(SCORUM_BLOGGING_START_DATE + 2 * SCORUM_BLOCK_INTERVAL);

        const auto& fund = reward_fund_sp_service.get();

        auto old_balance = fund.activity_reward_balance;

        generate_block();

        reward_sp_perblock = fund.activity_reward_balance - old_balance;

        BOOST_REQUIRE_GT(reward_sp_perblock, ASSET_NULL_SP);
    }

    ~find_asymptote_fixture()
    {
        scorum::protocol::detail::override_config(
            boost::make_unique<scorum::protocol::detail::config>(scorum::protocol::detail::config::test));
    }

    using actors_vector_type = std::vector<Actor>;

    share_type calculate_rshares_for_post(const Actor& author, const actors_vector_type& voters)
    {
        BOOST_REQUIRE(!voters.empty());

        auto post_permlink = create_next_post_permlink();

        const comment_object& post_obj = post(author, post_permlink); // alice post
        auto post_id = post_obj.id;

        const int vote_interval = SCORUM_CASHOUT_WINDOW_SECONDS / 2;

        generate_blocks(db.head_block_time() + vote_interval);

        for (const Actor& voter : voters)
        {
            vote(author, post_permlink, voter);

            generate_block();
        }

        share_type net_rshares = comment_service.get(post_id).net_rshares;

        fc::time_point_sec cashout = db.head_block_time();
        cashout += SCORUM_CASHOUT_WINDOW_SECONDS;
        cashout -= vote_interval;
        cashout -= SCORUM_BLOCK_INTERVAL * voters.size();

        generate_blocks(cashout);

        return net_rshares;
    }

    template <typename UntilLmb>
    bool find_asymptote(UntilLmb&& fn, int& equal_times, const int asymptote_detection_times = 3)
    {
        if (fn())
        {
            ++equal_times;
            if (equal_times > asymptote_detection_times)
                return true;
        }
        else if (equal_times > 0)
        {
            --equal_times;
        }
        return false;
    }

    void get_voting_point(percent_type& voting_power,
                          fc::time_point_sec& now,
                          fc::time_point_sec& last_vote,
                          int sec_delta,
                          const percent_type decay_percent = SCORUM_VOTING_POWER_DECAY_PERCENT)
    {
        percent_type current_power
            = calculate_restoring_power(voting_power, now, last_vote, SCORUM_VOTE_REGENERATION_SECONDS);
        percent_type used_power = calculate_used_power(voting_power, SCORUM_PERCENT(100), decay_percent);

        voting_power = current_power - used_power;

        last_vote = now;
        now += sec_delta;
    }

#ifdef PRINT_CURVE_POINTS
    const char* pstr_delimiter = ",";
    const char* pstr_endl = "\n";
#endif

    void plot_curve_of_voting_points(const int vote_period,
                                     const int max_iterations,
                                     const percent_type decay_percent = SCORUM_VOTING_POWER_DECAY_PERCENT)
    {
#ifdef PRINT_CURVE_POINTS
        using voting_powers_type = std::vector<voting_power_info>;
        voting_powers_type vps;
        vps.reserve(max_iterations);
        std::cerr << ">>> decay_percent = " << decay_percent << pstr_endl;
        std::cerr << "sequence" << pstr_delimiter << "time" << pstr_delimiter << "voting_power" << pstr_endl;
        std::cerr << "======================================" << pstr_endl;
#endif

        percent_type voting_power = SCORUM_100_PERCENT;
        fc::time_point_sec now = db.head_block_time();
        fc::time_point_sec last_vote_time = now - SCORUM_BLOCK_INTERVAL * 1000;

        percent_type prev_voting_power = 0;
        int equal_times = 0;
        int ci = 0;
        for (; ci < max_iterations; ++ci)
        {
            get_voting_point(voting_power, now, last_vote_time, vote_period, decay_percent);
            if (find_asymptote([=]() { return prev_voting_power == voting_power; }, equal_times))
                break;
#ifdef PRINT_CURVE_POINTS
            vps.emplace_back(voting_power, last_vote_time, ci);
#endif
            prev_voting_power = voting_power;
        }

#ifdef PRINT_CURVE_POINTS
        for (const voting_power_info& info : vps)
        {
            std::cerr << info.sequence << pstr_delimiter << info.time.to_iso_string() << pstr_delimiter
                      << info.voting_power * 100.f / SCORUM_100_PERCENT << pstr_endl;
        }
#endif

        BOOST_CHECK_LT(ci, max_iterations);
    }

    void process_per_block_without_cashout_comments(const share_type& reward_perblock,
                                                    share_type& reward_fund,
                                                    fc::uint128_t& total_claims,
                                                    fc::time_point_sec& now,
                                                    fc::time_point_sec& last_update)
    {
        total_claims
            = calculate_decreasing_total_claims(total_claims, now, last_update, SCORUM_RECENT_RSHARES_DECAY_RATE);

        last_update = now;
        now += SCORUM_BLOCK_INTERVAL;

        reward_fund += reward_perblock;
    }

    share_type process_per_block_for_cashout_comments(const share_type& rshares,
                                                      share_type& reward_fund,
                                                      fc::uint128_t& total_claims)
    {
        share_type payout;
        shares_vector_type total_rshares;

        total_rshares = { rshares };

        total_claims = calculate_total_claims(total_claims, curve_id::linear, total_rshares);

        payout = calculate_payout(rshares, total_claims, reward_fund, curve_id::linear,
                                  asset::maximum(SCORUM_SYMBOL).amount, SCORUM_MIN_COMMENT_PAYOUT_SHARE);

        reward_fund -= payout;

        return payout;
    }

    void plot_curve_of_payout_points(const int comment_cashout_period,
                                     const int max_iterations,
                                     const share_type rshares,
                                     const int initial_total_claims_decay)
    {
#ifdef PRINT_CURVE_POINTS
        using author_rewards_type = std::vector<payout_info>;
        author_rewards_type rewards;
        rewards.reserve(max_iterations);
        std::cerr << ">>> initial_total_claims_decay = " << initial_total_claims_decay << pstr_delimiter << "..."
                  << pstr_endl;
        std::cerr << "sequence" << pstr_delimiter << "payout_time" << pstr_delimiter << "total_claims" << pstr_delimiter
                  << "reward_fund" << pstr_delimiter << "payout" << pstr_endl;
        std::cerr << "======================================" << pstr_endl;
#endif

        fc::uint128_t total_claims;
        share_type reward_fund = 0;
        fc::time_point_sec last_payout = db.head_block_time();
        fc::time_point_sec now = last_payout + SCORUM_BLOCK_INTERVAL;

        BOOST_REQUIRE_GT(initial_total_claims_decay, 0);

        total_claims
            = fc::uint128_t(reward_sp_perblock.amount.value * fc::days(70).to_seconds() / SCORUM_BLOCK_INTERVAL);
        total_claims /= initial_total_claims_decay;

        int ci = 0;
        for (; ci < fc::days(7).to_seconds() / SCORUM_BLOCK_INTERVAL; ++ci)
        {
            process_per_block_without_cashout_comments(reward_sp_perblock.amount, reward_fund, total_claims, now,
                                                       last_payout);
        }

        share_type prev_payout;
        int equal_times = 0;
        for (ci = 0; ci < max_iterations; ++ci)
        {
            process_per_block_without_cashout_comments(reward_sp_perblock.amount, reward_fund, total_claims, now,
                                                       last_payout);
            if (ci % comment_cashout_period == 0)
            {
                auto payout = process_per_block_for_cashout_comments(rshares, reward_fund, total_claims);
                if (find_asymptote([=]() { return payout == prev_payout; }, equal_times))
                    break;
#ifdef PRINT_CURVE_POINTS
                rewards.emplace_back(reward_fund, total_claims, payout, last_payout, ci);
#endif
                prev_payout = payout;
            }
        }

#ifdef PRINT_CURVE_POINTS
        for (const payout_info& info : rewards)
        {
            std::cerr << info.sequence << pstr_delimiter << info.payout_time.to_iso_string() << pstr_delimiter
                      << (std::string)info.total_claims << pstr_delimiter << info.reward_fund.value << pstr_delimiter
                      << info.payout.value << pstr_endl;
        }
#endif

        BOOST_REQUIRE_LT(ci, max_iterations);
    }

    budget_service_i& budget_service;
    reward_fund_sp_service_i& reward_fund_sp_service;

    Actor alice;
    Actor bob;
    Actor sam;
    Actor simon;

    asset reward_sp_perblock = ASSET_NULL_SP;
};

BOOST_FIXTURE_TEST_SUITE(find_asymptote_tests, find_asymptote_fixture)

// BOOST_AUTO_TEST_CASE(find_asymptote_in_voting_power_decreasing_default)
//{
//    const int vote_period = fc::minutes(60).to_seconds();
//    const int max_iterations = fc::days(7).to_seconds() / vote_period;

//    plot_curve_of_voting_points(vote_period, max_iterations);
//}

// BOOST_AUTO_TEST_CASE(find_asymptote_in_voting_power_decreasing_multy_coeff)
//{
//    const int vote_period = fc::minutes(1).to_seconds();
//    const int max_iterations = fc::days(14).to_seconds() / vote_period;

//    plot_curve_of_voting_points(vote_period, max_iterations, 3);
//    plot_curve_of_voting_points(vote_period, max_iterations, 4);
//    plot_curve_of_voting_points(vote_period, max_iterations, 5);
//}

BOOST_AUTO_TEST_CASE(find_asymptote_in_payout_alg)
{
    // 1.5 year
    const int max_iterations = fc::days(365 * 3 / 2).to_seconds() / SCORUM_BLOCK_INTERVAL;
    // cashout each 30 minutes
    const int comment_cashout_period = fc::minutes(10).to_seconds() / SCORUM_BLOCK_INTERVAL;

    share_type rshares = calculate_rshares_for_post(alice, { bob, sam });

    BOOST_REQUIRE_GT(rshares, share_type());

    //    plot_curve_of_payout_points(comment_cashout_period, max_iterations, rshares,
    //                                reward_fund_sp_service.get().recent_claims);

    plot_curve_of_payout_points(comment_cashout_period, max_iterations, rshares, 1);
}

BOOST_AUTO_TEST_SUITE_END()
}
