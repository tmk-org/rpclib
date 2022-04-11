#include "rpc/detail/func_tools.h"
#include "rpc/detail/func_traits.h"
namespace rpc {

template<typename...T> struct RefArgsHandlerClientSide;
template<typename...TInner,size_t...Inneridxs> struct RefArgsHandlerClientSide<std::tuple<TInner...>,std::index_sequence<Inneridxs...>>
{
    template< typename... outerArgs > 
    static RPCLIB_MSGPACK::object_handle HandleRefArgs(std::future<RPCLIB_MSGPACK::object_handle> future,outerArgs&...args)
    {
        using Producer=detail::RefArgsProducer<std::tuple<std::decay_t<outerArgs>>...>;
        auto ret_object= future.get();
        auto object=ret_object.get();
        auto rv_obj = std::tuple<std::decay_t<TInner>...>{};
        //ret_value_type rv_obj={};
        object.convert(rv_obj);
        Producer::template  ReprojectTuple<std::tuple<TInner...>,std::index_sequence<Inneridxs...> >::
                            ReprojectBack(std::tie(args...),std::move(rv_obj));
        return ret_object;
    }
};

template<   typename... Args,
            typename ref_pointer_tuple,
            typename get_idxs_seq> 
RPCLIB_MSGPACK::object_handle HandleRefArgs(std::future<RPCLIB_MSGPACK::object_handle> future,Args&...args)
{
    auto ret_object= future.get();
    return ret_object;
}



template <typename... Args>
RPCLIB_MSGPACK::object_handle client::call( std::string const &func_name,
                                            Args... args) {
    RPCLIB_CREATE_LOG_CHANNEL(client)
    auto future = async_call(func_name, std::forward<Args>(args)...);
    if (auto timeout = get_timeout()) {
        auto wait_result = future.wait_for(std::chrono::milliseconds(*timeout));
        if (wait_result == std::future_status::timeout) {
            throw_timeout(func_name);
        }
    }
    if constexpr(detail::pack_has_references<Args...>::value)
    {
        using merged_args_type = detail::ResultTraits<void,Args...>::type;
        using ref_args_type = detail::ReferenceTupleElementHandler< merged_args_type > ::type;
        using args_type = std::tuple<std::decay_t<Args>...>;
        using producer = detail::RefArgsProducer<args_type>;
        using retriever = producer::template RefArgsPointer<ref_args_type>;
        using ref_args_type_tuple = typename retriever::tuple_type;
        using ref_args_type_tuple_seq = typename retriever::sequence_type;
        return RefArgsHandlerClientSide<ref_args_type_tuple,ref_args_type_tuple_seq>::
            template HandleRefArgs(std::move(future),/*std::ref(*/args/*)*/...);
        //return HandleRefArgs(std::move(future),std::ref(args)...);
    }
    return future.get();
}

template <typename... Args>
std::future<RPCLIB_MSGPACK::object_handle>
client::async_call(std::string const &func_name, Args... args) {
    RPCLIB_CREATE_LOG_CHANNEL(client)
    wait_conn();
    using RPCLIB_MSGPACK::object;
    LOG_DEBUG("Calling {}", func_name);

    auto args_obj = std::make_tuple(args...);
    const int idx = get_next_call_idx();
    auto call_obj =
        std::make_tuple(static_cast<uint8_t>(client::request_type::call), idx,
                        func_name, args_obj);

    auto buffer = std::make_shared<RPCLIB_MSGPACK::sbuffer>();
    RPCLIB_MSGPACK::pack(*buffer, call_obj);

    // TODO: Change to move semantics when asio starts supporting move-only
    // handlers in post(). [sztomi, 2016-02-14]
    auto p = std::make_shared<std::promise<RPCLIB_MSGPACK::object_handle>>();
    auto ft = p->get_future();

    post(buffer, idx, func_name, p);

    return ft;
}

//! \brief Sends a notification with the given name and arguments (if any).
//! \param func_name The name of the notification to call.
//! \param args The arguments to pass to the function.
//! \note This function returns when the notification is written to the
//! socket.
//! \tparam Args The types of the arguments.
template <typename... Args>
void client::send(std::string const &func_name, Args... args) {
    RPCLIB_CREATE_LOG_CHANNEL(client)
    LOG_DEBUG("Sending notification {}", func_name);

    auto args_obj = std::make_tuple(args...);
    auto call_obj = std::make_tuple(
        static_cast<uint8_t>(client::request_type::notification), func_name,
        args_obj);

    auto buffer = new RPCLIB_MSGPACK::sbuffer;
    RPCLIB_MSGPACK::pack(*buffer, call_obj);

    post(buffer);
}
}
