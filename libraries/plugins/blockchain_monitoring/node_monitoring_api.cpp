#include <scorum/blockchain_monitoring/node_monitoring_api.hpp>
#include <scorum/blockchain_monitoring/blockchain_monitoring_plugin.hpp>

namespace scorum {
namespace blockchain_monitoring {

namespace detail {
class node_monitoring_api_impl
{

public:
    scorum::app::application& _app;

public:
    node_monitoring_api_impl(scorum::app::application& app)
        : _app(app)
    {
    }

    std::shared_ptr<blockchain_monitoring_plugin> get_plugin() const
    {
        auto plugin = _app.get_plugin<blockchain_monitoring_plugin>(BLOCKCHAIN_MONITORING_PLUGIN_NAME);

        FC_ASSERT(plugin, "Cann't get " BLOCKCHAIN_MONITORING_PLUGIN_NAME " plugin from application.");

        return plugin;
    }
};
} // namespace detail

node_monitoring_api::node_monitoring_api(const scorum::app::api_context& ctx)
{
    _my = std::make_shared<detail::node_monitoring_api_impl>(ctx.app);
}

void node_monitoring_api::on_api_startup()
{
}

uint32_t node_monitoring_api::get_last_block_duration_microseconds() const
{
    return _my->_app.chain_database()->with_read_lock(
        [&]() { return _my->get_plugin()->get_last_block_duration_microseconds(); });
}

uint32_t node_monitoring_api::get_free_shared_memory_mb() const
{
    return _my->_app.chain_database()->with_read_lock(
        [&]() { return uint32_t(_my->_app.chain_database()->get_free_memory() / (1024 * 1024)); });
}
uint32_t node_monitoring_api::get_total_shared_memory_mb() const
{
    return 0;
    //    return _my->_app.chain_database()->with_read_lock(
    //        [&]() { return uint32_t(_my->_app.chain_database()->get_size() / (1024 * 1024)); });
}

} // namespace blockchain_monitoring
} // namespace scorum
