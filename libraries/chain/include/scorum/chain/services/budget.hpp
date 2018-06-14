#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/budget_object.hpp>
namespace scorum {
namespace chain {

struct budget_service_i : public base_service_i<budget_object>
{
    using budget_refs_type = std::vector<std::reference_wrapper<const budget_object>>;

    virtual std::set<std::string> lookup_budget_owners(const std::string& lower_bound_owner_name,
                                                       uint32_t limit) const = 0;
    virtual budget_refs_type get_budgets() const = 0;
    virtual budget_refs_type get_top_budgets(const uint16_t limit) const = 0;
    virtual budget_refs_type get_budgets(const account_name_type& owner) const = 0;
    virtual const budget_object& get_budget(budget_id_type id) const = 0;
    virtual const budget_object& create_budget(const account_object& owner,
                                               const asset& balance,
                                               const time_point_sec& deadline,
                                               const optional<std::string>& content_permlink = optional<std::string>())
        = 0;
    virtual void close_budget(const budget_object& budget) = 0;

    virtual const budget_object& create_fund_budget(const asset& balance, const time_point_sec& deadline) = 0;
    virtual const budget_object& get_fund_budget() const = 0;
    virtual bool is_fund_budget_exists() const = 0;

    virtual asset allocate_cash(const budget_object& budget) = 0;
};

/**
 * DB service for operations with budget_object
 */
class dbs_budget : public dbs_service_base<budget_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_budget(database& db);

public:
    bool is_fund_budget_exists() const override;

    virtual std::set<std::string> lookup_budget_owners(const std::string& lower_bound_owner_name,
                                                       uint32_t limit) const override;

    /** Lists all owned budgets.
     *
     * @returns a list of budget objects
     */
    virtual budget_refs_type get_budgets() const override;

    virtual budget_refs_type get_top_budgets(const uint16_t limit) const override;

    /** Lists all budgets registered for owner.
     *
     * @param owner the name of the owner
     * @returns a list of budget objects
     */
    virtual budget_refs_type get_budgets(const account_name_type& owner) const override;

    /** Gets the fund budget
     */
    virtual const budget_object& get_fund_budget() const override;

    /** Get budget by id
     */
    virtual const budget_object& get_budget(budget_id_type id) const override;

    /** Creates fund budget (no owners).
     *
     * @warning count of fund budgets must be less or equal than SCORUM_BUDGETS_LIMIT_PER_OWNER.
     *
     * @param balance the total balance (in SCR)
     * @param deadline the deadline time to close budget (even if there is rest of balance)
     * @returns fund budget object
     */
    virtual const budget_object& create_fund_budget(const asset& balance, const time_point_sec& deadline) override;

    /** Creates budget.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param balance the total balance (in SCR)
     * @param deadline the deadline time to close budget (even if there is rest of balance)
     * @param content_permlink the budget target identity (post or other)
     * @returns a budget object
     */
    virtual const budget_object& create_budget(const account_object& owner,
                                               const asset& balance,
                                               const time_point_sec& deadline,
                                               const optional<std::string>& content_permlink
                                               = optional<std::string>()) override;

    /** Closing budget.
     *  To delete the budget, to cash back from budget to owner account.
     *
     * @warning It is not allowed for fund budget.
     *
     * @param budget the budget to close
     */
    virtual void close_budget(const budget_object& budget);

    /** Distributes asset from budget.
     *  This operation takes into account the deadline and last block number
     *
     * @param budget the budget that is distributed
     */
    virtual asset allocate_cash(const budget_object& budget) override;

private:
    share_type _calculate_per_block(const time_point_sec& start_date,
                                    const time_point_sec& end_date,
                                    const share_type& balance_amount);
    share_type _decrease_balance(const budget_object&, const share_type& balance);
    bool _is_fund_budget(const budget_object&) const;
    void _close_budget(const budget_object&);
    void _close_owned_budget(const budget_object&);
    void _close_fund_budget(const budget_object&);
    uint64_t _get_budget_count(const account_name_type& owner) const;
    const budget_object& _create_budget(const account_name_type& owner,
                                        const share_type& balance,
                                        const time_point_sec& start_date,
                                        const time_point_sec& end_date,
                                        const optional<std::string>& content_permlink = optional<std::string>());
};
} // namespace chain
} // namespace scorum
