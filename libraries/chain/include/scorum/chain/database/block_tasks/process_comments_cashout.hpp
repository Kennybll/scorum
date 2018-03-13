#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/services/comment.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

struct process_comments_cashout : public block_task
{
    virtual void on_apply(block_task_context&);

private:
    uint128_t get_recent_claims(block_task_context& ctx, const comment_service_i::comment_refs_type&);

    asset pay_for_comment(block_task_context& ctx, const comment_object& comment, const asset& reward);

    asset pay_curators(block_task_context& ctx, const comment_object& comment, asset& max_rewards);
};
}
}
}
