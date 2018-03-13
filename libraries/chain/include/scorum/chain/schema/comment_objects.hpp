#pragma once

#include <fc/shared_string.hpp>
#include <fc/shared_buffer.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <limits>

namespace scorum {
namespace chain {

using protocol::beneficiary_route_type;

class comment_object : public object<comment_object_type, comment_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(
        comment_object, (category)(parent_permlink)(permlink)(title)(body)(json_metadata)(beneficiaries))

    id_type id;

    fc::shared_string category;
    account_name_type parent_author;
    fc::shared_string parent_permlink;
    account_name_type author;
    fc::shared_string permlink;

    fc::shared_string title;
    fc::shared_string body;
    fc::shared_string json_metadata;
    time_point_sec last_update;
    time_point_sec created;
    time_point_sec active; ///< the last time this post was "touched" by voting or reply
    time_point_sec last_payout;

    uint16_t depth = 0; ///< used to track max nested depth
    uint32_t children = 0; ///< used to track the total number of children, grandchildren, etc...

    share_type net_rshares; /// This is the sum of all votes (positive and negative).
    /// Used for calculation reward witch is proportional to rshares^2
    share_type abs_rshares; /// This is used to track the total abs(weight) of votes.
    share_type vote_rshares; /// Total positive rshares from all votes.
    /// Used to calculate delta weights. Needed to handle vote changing and removal.

    share_type children_abs_rshares; /// this is used to calculate cashout time of a discussion.
    time_point_sec cashout_time; /// 24 hours from the weighted average of vote time
    time_point_sec max_cashout_time;
    uint64_t total_vote_weight
        = 0; /// the total weight of voting rewards, used to calculate pro-rata share of curation payouts

    uint16_t reward_weight = 0;

    /** tracks the total payout this comment has received over time, measured in SCR */
    asset total_payout_value = asset(0, SCORUM_SYMBOL);
    asset curator_payout_value = asset(0, SCORUM_SYMBOL);
    asset beneficiary_payout_value = asset(0, SCORUM_SYMBOL);

    asset author_rewards = asset(0, SCORUM_SYMBOL);

    int32_t net_votes = 0;

    id_type root_comment;

    asset max_accepted_payout
        = asset::maximum(SCORUM_SYMBOL); /// SCR value of the maximum payout this post will receive
    uint16_t percent_scrs = SCORUM_100_PERCENT; /// the percent of SCR to pay, unkept amounts will be received as SP
    bool allow_replies = true; /// allows a post to disable replies.
    bool allow_votes = true; /// allows a post to receive votes;
    bool allow_curation_rewards = true;

    fc::shared_vector<beneficiary_route_type> beneficiaries;
};

/**
 * This index maintains the set of voter/comment pairs that have been used, voters cannot
 * vote on the same comment more than once per payout period.
 */
class comment_vote_object : public object<comment_vote_object_type, comment_vote_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(comment_vote_object)

    id_type id;

    account_id_type voter;
    comment_id_type comment;
    uint64_t weight
        = 0; ///< defines the score this vote receives, used by vote payout calc. 0 if a negative vote or changed votes.
    int64_t rshares = 0; ///< The number of rshares this vote is responsible for
    int16_t vote_percent = 0; ///< The percent weight of the vote
    time_point_sec last_update; ///< The time of the last update of the vote
    int8_t num_changes = 0;
};

// clang-format off

struct by_comment_voter;
struct by_voter_comment;
struct by_comment_weight_voter;
struct by_voter_last_update;
typedef shared_multi_index_container<comment_vote_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<comment_vote_object,
                                                               comment_vote_id_type,
                                                               &comment_vote_object::id>>,
                                         ordered_unique<tag<by_comment_voter>,
                                                        composite_key<comment_vote_object,
                                                                      member<comment_vote_object,
                                                                             comment_id_type,
                                                                             &comment_vote_object::comment>,
                                                                      member<comment_vote_object,
                                                                             account_id_type,
                                                                             &comment_vote_object::voter>>>,
                                         ordered_unique<tag<by_voter_comment>,
                                                        composite_key<comment_vote_object,
                                                                      member<comment_vote_object,
                                                                             account_id_type,
                                                                             &comment_vote_object::voter>,
                                                                      member<comment_vote_object,
                                                                             comment_id_type,
                                                                             &comment_vote_object::comment>>>,
                                         ordered_unique<tag<by_voter_last_update>,
                                                        composite_key<comment_vote_object,
                                                                      member<comment_vote_object,
                                                                             account_id_type,
                                                                             &comment_vote_object::voter>,
                                                                      member<comment_vote_object,
                                                                             time_point_sec,
                                                                             &comment_vote_object::last_update>,
                                                                      member<comment_vote_object,
                                                                             comment_id_type,
                                                                             &comment_vote_object::comment>>,
                                                        composite_key_compare<std::less<account_id_type>,
                                                                              std::greater<time_point_sec>,
                                                                              std::less<comment_id_type>>>,
                                         ordered_unique<tag<by_comment_weight_voter>,
                                                        composite_key<comment_vote_object,
                                                                      member<comment_vote_object,
                                                                             comment_id_type,
                                                                             &comment_vote_object::comment>,
                                                                      member<comment_vote_object,
                                                                             uint64_t,
                                                                             &comment_vote_object::weight>,
                                                                      member<comment_vote_object,
                                                                             account_id_type,
                                                                             &comment_vote_object::voter>>,
                                                        composite_key_compare<std::less<comment_id_type>,
                                                                              std::greater<uint64_t>,
                                                                              std::less<account_id_type>>>>
     >
    comment_vote_index;

struct by_cashout_time; /// cashout_time
struct by_permlink; /// author, perm
struct by_root;
struct by_parent;
struct by_active; /// parent_auth, active
struct by_pending_payout;
struct by_total_pending_payout;
struct by_last_update; /// parent_auth, last_update
struct by_created; /// parent_auth, last_update
struct by_payout; /// parent_auth, last_update
struct by_blog;
struct by_votes;
struct by_responses;
struct by_author_last_update;

/**
 * @ingroup object_index
 */
typedef shared_multi_index_container<comment_object,
                              indexed_by<
                                  /// CONSENUSS INDICIES - used by evaluators
                                  ordered_unique<tag<by_id>,
                                                 member<comment_object, comment_id_type, &comment_object::id>>,
                                  ordered_unique<tag<by_cashout_time>,
                                                 composite_key<comment_object,
                                                               member<comment_object,
                                                                      time_point_sec,
                                                                      &comment_object::cashout_time>,
                                                               member<comment_object,
                                                                      comment_id_type,
                                                                      &comment_object::id>>>,
                                  ordered_unique<tag<by_permlink>, /// used by consensus to find posts referenced in ops
                                                 composite_key<comment_object,
                                                               member<comment_object,
                                                                      account_name_type,
                                                                      &comment_object::author>,
                                                               member<comment_object,
                                                                      fc::shared_string,
                                                                      &comment_object::permlink>>,
                                                 composite_key_compare<std::less<account_name_type>, fc::strcmp_less>>,
                                  ordered_unique<tag<by_root>,
                                                 composite_key<comment_object,
                                                               member<comment_object,
                                                                      comment_id_type,
                                                                      &comment_object::root_comment>,
                                                               member<comment_object,
                                                                      comment_id_type,
                                                                      &comment_object::id>>>,
                                  ordered_unique<tag<by_parent>, /// used by consensus to find posts referenced in ops
                                                 composite_key<comment_object,
                                                               member<comment_object,
                                                                      account_name_type,
                                                                      &comment_object::parent_author>,
                                                               member<comment_object,
                                                                      fc::shared_string,
                                                                      &comment_object::parent_permlink>,
                                                               member<comment_object,
                                                                      comment_id_type,
                                                                      &comment_object::id>>,
                                                 composite_key_compare<std::less<account_name_type>,
                                                                       fc::strcmp_less,
                                                                       std::less<comment_id_type>>>
/// NON_CONSENSUS INDICIES - used by APIs
#ifndef IS_LOW_MEM
                                  ,
                                  ordered_unique<tag<by_last_update>,
                                                 composite_key<comment_object,
                                                               member<comment_object,
                                                                      account_name_type,
                                                                      &comment_object::parent_author>,
                                                               member<comment_object,
                                                                      time_point_sec,
                                                                      &comment_object::last_update>,
                                                               member<comment_object,
                                                                      comment_id_type,
                                                                      &comment_object::id>>,
                                                 composite_key_compare<std::less<account_name_type>,
                                                                       std::greater<time_point_sec>,
                                                                       std::less<comment_id_type>>>,
                                  ordered_unique<tag<by_author_last_update>,
                                                 composite_key<comment_object,
                                                               member<comment_object,
                                                                      account_name_type,
                                                                      &comment_object::author>,
                                                               member<comment_object,
                                                                      time_point_sec,
                                                                      &comment_object::last_update>,
                                                               member<comment_object,
                                                                      comment_id_type,
                                                                      &comment_object::id>>,
                                                 composite_key_compare<std::less<account_name_type>,
                                                                       std::greater<time_point_sec>,
                                                                       std::less<comment_id_type>>>
#endif
                                  >
    >
    comment_index;

// clang-format on
} // namespace chain
} // namespace scorum

// clang-format off

FC_REFLECT( scorum::chain::comment_object,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (title)(body)(json_metadata)(last_update)(created)(active)(last_payout)
             (depth)(children)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)(max_cashout_time)
             (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(beneficiary_payout_value)(author_rewards)(net_votes)(root_comment)
             (max_accepted_payout)(percent_scrs)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::comment_object, scorum::chain::comment_index )

FC_REFLECT( scorum::chain::comment_vote_object,
             (id)(voter)(comment)(weight)(rshares)(vote_percent)(last_update)(num_changes)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::comment_vote_object, scorum::chain::comment_vote_index )

// clang-format on
