#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/hardfork.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/util/reward.hpp>

#include <scorum/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>
#include "database_trx_integration.hpp"

#include <sstream>
#include <cmath>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;

namespace database_fixture {
struct reward_fund_integration_fixture : public database_trx_integration_fixture
{
    const asset account_initial_vest_supply = ASSET_SP(100e+9); // 100.0 SP

    reward_fund_integration_fixture()
        : alice("alice")
        , bob("bob")
        , account_service(db.account_service())
        , dgp_service(db.dynamic_global_property_service())
    {
        // switch off registration pool to get 0 SP for both alice and bob acounts
        genesis_state_type genesis = Genesis::create()
                                         .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY)
                                         .rewards_supply(ASSET_SCR(4800000e+9))
                                         .witnesses(initdelegate)
                                         .accounts(alice, bob)
                                         .dev_committee(initdelegate)
                                         .generate();
        open_database(genesis);

        actor(initdelegate).give_sp(alice, account_initial_vest_supply.amount.value);
        actor(initdelegate).give_sp(bob, account_initial_vest_supply.amount.value);

        generate_block();
    }

    Actor alice;
    Actor bob;

    account_service_i& account_service;
    dynamic_global_property_service_i& dgp_service;
};
}

BOOST_AUTO_TEST_SUITE(reward_fund_tests)

BOOST_FIXTURE_TEST_CASE(reward_fund, database_fixture::reward_fund_integration_fixture)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: reward_fund");

        const auto blocks_between_comments = 5;

        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("alice").balance, asset(0, SCORUM_SYMBOL));
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").balance, asset(0, SCORUM_SYMBOL));
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("alice").scorumpower,
                            account_initial_vest_supply);
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").scorumpower,
                            account_initial_vest_supply);

        comment_operation comment;
        vote_operation vote;
        signed_transaction tx;

        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "foo";
        comment.body = "bar";
        vote.voter = "alice";
        vote.author = "alice";
        vote.permlink = "test";
        vote.weight = (int16_t)100;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice.private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(blocks_between_comments);

        comment.author = "bob";
        comment.parent_author = "alice";
        vote.voter = "bob";
        vote.author = "bob";
        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(bob.private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const auto& fund = db.obtain_service<dbs_reward_fund>().get();

        BOOST_REQUIRE_GT(fund.activity_reward_balance_scr, asset(0, SCORUM_SYMBOL));
        BOOST_REQUIRE_EQUAL(fund.recent_claims.to_uint64(), uint64_t(0));

        share_type alice_comment_net_rshares
            = db.obtain_service<dbs_comment>().get("alice", std::string("test")).net_rshares;
        share_type bob_comment_net_rshares
            = db.obtain_service<dbs_comment>().get("bob", std::string("test")).net_rshares;

        {
            generate_blocks(db.obtain_service<dbs_comment>().get("alice", std::string("test")).cashout_time);

            BOOST_REQUIRE_EQUAL(fund.activity_reward_balance_scr, ASSET_SCR(0));
            BOOST_REQUIRE_EQUAL(fund.recent_claims.to_uint64(), alice_comment_net_rshares);

            // clang-format off
            BOOST_REQUIRE_GT   (db.obtain_service<dbs_account>().get_account("alice").scorumpower, account_initial_vest_supply);
            BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("alice").scorumpower,
                                account_initial_vest_supply + ASSET_SP(db.obtain_service<dbs_account>().get_account("alice").balance.amount.value));

            BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").scorumpower, account_initial_vest_supply);
            BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").scorumpower,
                                account_initial_vest_supply + ASSET_SP(db.obtain_service<dbs_account>().get_account("bob").balance.amount.value));
            // clang-format on

            validate_database();
        }

        {
            generate_blocks(blocks_between_comments);

            for (auto i = 0; i < blocks_between_comments; ++i)
            {
                share_type decay
                    = alice_comment_net_rshares * SCORUM_BLOCK_INTERVAL / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
                alice_comment_net_rshares -= decay;
            }

            BOOST_REQUIRE_EQUAL(fund.recent_claims.to_uint64(), alice_comment_net_rshares + bob_comment_net_rshares);
            BOOST_REQUIRE_GT(fund.activity_reward_balance_scr, ASSET_SCR(0));

            BOOST_REQUIRE_GT(db.obtain_service<dbs_account>().get_account("alice").scorumpower,
                             account_initial_vest_supply);
            BOOST_REQUIRE_GT(db.obtain_service<dbs_account>().get_account("bob").scorumpower,
                             account_initial_vest_supply);

            validate_database();
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(recent_claims_decay, database_fixture::reward_fund_integration_fixture)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: recent_rshares_2decay");

        comment_operation comment;
        vote_operation vote;
        signed_transaction tx;

        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "foo";
        comment.body = "bar";
        vote.voter = "alice";
        vote.author = "alice";
        vote.permlink = "test";
        vote.weight = (int16_t)100;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice.private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto alice_vshares = util::evaluate_reward_curve(
            db.obtain_service<dbs_comment>().get("alice", std::string("test")).net_rshares.value,
            db.obtain_service<dbs_reward_fund>().get().author_reward_curve);

        generate_blocks(5);

        comment.author = "bob";
        vote.voter = "bob";
        vote.author = "bob";
        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(bob.private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(db.obtain_service<dbs_comment>().get("alice", std::string("test")).cashout_time);

        {
            const auto& post_rf = db.obtain_service<dbs_reward_fund>().get();

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares);
            validate_database();
        }

        auto bob_cashout_time = db.obtain_service<dbs_comment>().get("bob", std::string("test")).cashout_time;
        auto bob_vshares = util::evaluate_reward_curve(
            db.obtain_service<dbs_comment>().get("bob", std::string("test")).net_rshares.value,
            db.obtain_service<dbs_reward_fund>().get().author_reward_curve);

        while (db.head_block_time() < bob_cashout_time - SCORUM_BLOCK_INTERVAL)
        {
            generate_block();

            alice_vshares -= (alice_vshares * SCORUM_BLOCK_INTERVAL) / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            const auto& post_rf = db.obtain_service<dbs_reward_fund>().get();

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares);
        }

        generate_block();

        {
            alice_vshares -= (alice_vshares * SCORUM_BLOCK_INTERVAL) / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            const auto& post_rf = db.obtain_service<dbs_reward_fund>().get();

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares + bob_vshares);
            validate_database();
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

namespace database_fixture {
struct reward_fund_sequence_fixture : public reward_fund_integration_fixture
{
    reward_fund_sequence_fixture()
        : sam("sam")
        , sam2("sam2")
        , jon("jon")
        , smit("smit")
        , reward_fund_service(db.reward_fund_service())
        , comment_service(db.comment_service())
        , comment_vote_service(db.comment_vote_service())
    {
        actor(initdelegate).create_account(sam);
        actor(initdelegate).give_sp(sam, account_initial_vest_supply.amount.value);

        actor(initdelegate).create_account(sam2);
        actor(initdelegate).give_sp(sam2, account_initial_vest_supply.amount.value);

        actor(initdelegate).create_account(jon);
        actor(initdelegate).give_sp(jon, account_initial_vest_supply.amount.value);

        actor(initdelegate).create_account(smit);
        actor(initdelegate).give_sp(smit, account_initial_vest_supply.amount.value);

        generate_block();
    }

    std::string create_next_permlink()
    {
        static int next = 0;
        std::stringstream store;
        store << "blog-" << ++next;
        return store.str();
    }

    std::string get_comment_permlink(const std::string& permlink)
    {
        return std::string("re-") + permlink;
    }

    void take_initial_reward()
    {
        comment_operation comment;

        comment.author = jon.name;
        comment.permlink = create_next_permlink();
        comment.parent_permlink = "posts";
        comment.title = "foo";
        comment.body = "bar";

        push_operation_only(comment, jon.private_key);

        auto cashout_time = comment_service.get(jon.name, comment.permlink).cashout_time;

        vote_operation vote;

        vote.voter = smit.name;
        vote.author = jon.name;
        vote.permlink = comment.permlink;
        vote.weight = (int16_t)100;

        push_operation_only(vote, smit.private_key);

        generate_blocks_to_next_timeline(cashout_time);
    }

    comment_object post(const std::string& permlink)
    {
        comment_operation comment;

        comment.author = alice.name;
        comment.permlink = permlink;
        comment.parent_permlink = "posts";
        comment.title = "foo";
        comment.body = "bar";

        push_operation_only(comment, alice.private_key);

        return comment_service.get(alice.name, permlink);
    }

    comment_object comment(const std::string& permlink)
    {
        comment_operation comment;

        comment.author = bob.name;
        comment.permlink = get_comment_permlink(permlink);
        comment.parent_author = alice.name;
        comment.parent_permlink = permlink;
        comment.title = "re: foo";
        comment.body = "re: bar";

        push_operation_only(comment, bob.private_key);

        return comment_service.get(bob.name, comment.permlink);
    }

    void vote_for_post(const std::string& permlink)
    {
        vote_operation vote;

        vote.voter = sam.name;
        vote.author = alice.name;
        vote.permlink = permlink;
        vote.weight = (int16_t)100;

        push_operation_only(vote, sam.private_key);
    }

    void vote_for_comment(const std::string& permlink)
    {
        vote_operation vote;

        vote.voter = sam2.name;
        vote.author = bob.name;
        vote.permlink = get_comment_permlink(permlink);
        vote.weight = (int16_t)100;

        push_operation_only(vote, sam2.private_key);
    }

    void stat_reward_fund()
    {
        auto block_num = dgp_service.get().head_block_number;
        auto activity_reward_balance_scr = reward_fund_service.get().activity_reward_balance_scr;
        auto recent_claims = reward_fund_service.get().recent_claims;

        wlog(">> ${t} #${n}: rf = ${rf}, rc = ${rc}", ("t", dgp_service.head_block_time())("n", block_num)(
                                                          "rf", activity_reward_balance_scr)("rc", recent_claims));
    }

    using recent_funds_type = std::map<std::string, asset>;

    void stat_account_funds(const Actor& account, recent_funds_type& recent_scr, recent_funds_type& recent_sp)
    {
        if (recent_scr.find(account) == recent_scr.end())
            recent_scr[account] = ASSET_SCR(0);

        if (recent_sp.find(account) == recent_sp.end())
            recent_sp[account] = ASSET_SP(0);

        const auto& account_obj = account_service.get_account(account.name);
        wlog("${name}: delta: ${d_scr}, ${d_sp}; total: ${scr}, ${sp}",
             ("name", account.name)("d_scr", account_obj.balance - recent_scr[account])(
                 "d_sp", account_obj.scorumpower - recent_sp[account])("scr", account_obj.balance)(
                 "sp", account_obj.scorumpower));

        recent_scr[account] = account_obj.balance;
        recent_sp[account] = account_obj.scorumpower;
    }

    void stat_comment(const comment_object& comment)
    {
        wlog("${n}-${p}: net_rshares: ${rs}", ("n", comment.author)("p", comment.permlink)("rs", comment.net_rshares));

        auto comment_votes = comment_vote_service.get_by_comment_weight_voter(comment.id);
        for (const comment_vote_object& vote : comment_votes)
        {
            wlog("total_vote_weight: ${t}, weight: ${w}", ("t", comment.total_vote_weight)("w", vote.weight));
        }
    }

    void generate_blocks_to_next_timeline(const fc::time_point_sec& timeline)
    {
        stat_reward_fund();
        generate_blocks(timeline, false);
        stat_reward_fund();
    }

    Actor sam;
    Actor sam2;
    Actor jon;
    Actor smit;

    reward_fund_service_i& reward_fund_service;
    comment_service_i& comment_service;
    comment_vote_service_i& comment_vote_service;
};
}

BOOST_AUTO_TEST_SUITE(reward_fund_sequence_tests)

BOOST_FIXTURE_TEST_CASE(recent_claims_long_decay, database_fixture::reward_fund_sequence_fixture)
{
    const int seq_n = 20;
    int ci = 0;

    generate_blocks_to_next_timeline(
        fc::time_point_sec(dgp_service.head_block_time().sec_since_epoch() + fc::hours(8).to_seconds()));

    take_initial_reward();

    recent_funds_type recent_scr;
    recent_funds_type recent_sp;

    while (ci++ < seq_n)
    {
        stat_account_funds(alice, recent_scr, recent_sp);
        stat_account_funds(bob, recent_scr, recent_sp);
        stat_account_funds(sam, recent_scr, recent_sp);
        stat_account_funds(sam2, recent_scr, recent_sp);

        auto post_permlink = create_next_permlink();

        const comment_object& comment_post = post(post_permlink);

        auto delta = (comment_post.cashout_time - dgp_service.head_block_time()).to_seconds();

        generate_blocks_to_next_timeline(
            fc::time_point_sec(dgp_service.head_block_time().sec_since_epoch() + delta / 3));

        const comment_object& comment_comment = comment(post_permlink);

        delta = (comment_comment.cashout_time - comment_post.cashout_time).to_seconds();

        generate_blocks_to_next_timeline(
            fc::time_point_sec(dgp_service.head_block_time().sec_since_epoch() + 2 * delta / 3));

        vote_for_post(post_permlink);
        vote_for_comment(post_permlink);

        stat_comment(db.obtain_service<dbs_comment>().get(alice.name, post_permlink));
        stat_comment(db.obtain_service<dbs_comment>().get(bob.name, get_comment_permlink(post_permlink)));

        generate_blocks_to_next_timeline(comment_post.cashout_time);

        generate_blocks_to_next_timeline(comment_comment.cashout_time);
    }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
