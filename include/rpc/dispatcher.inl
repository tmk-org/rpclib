
namespace rpc {
namespace detail {

template <typename F > void dispatcher::bind(std::string const &name, F func) {
    enforce_unique_name(name);
    if constexpr (!has_ref_args<F>::value)
    {
        bind(name, func, typename detail::func_kind_info<F>::result_kind(),
            typename detail::func_kind_info<F>::args_kind());
    }
    else
    {
        bind(name, func, typename detail::func_kind_info<F>::result_kind(),
            typename detail::func_kind_info<F>::args_kind(),detail::tags::refs_args{});
    }
    
}
/*
template <typename F > std::enable_if_t<!has_ref_args<F>::value,void> dispatcher::bind(std::string const &name, F func) {
    enforce_unique_name(name);
    bind(name, func, typename detail::func_kind_info<F>::result_kind(),
         typename detail::func_kind_info<F>::args_kind());
}*/
/*
template <typename F > void dispatcher::bind(std::string const &name, std::enable_if_t<!has_ref_args<F>::value,F > func) {
    enforce_unique_name(name);
    bind(name, func, typename detail::func_kind_info<F>::result_kind(),
         typename detail::func_kind_info<F>::args_kind());
}*/

template <typename F>
void dispatcher::bind(std::string const &name, F func,
                      detail::tags::void_result const &,
                      detail::tags::zero_arg const &) {
    enforce_unique_name(name);
    funcs_.insert(
        std::make_pair(name, [func, name](RPCLIB_MSGPACK::object const &args) {
            enforce_arg_count(name, 0, args.via.array.size);
            func();
            return rpc::detail::make_unique<RPCLIB_MSGPACK::object_handle>();
        }));
}

template <typename F>
void dispatcher::bind(std::string const &name, F func,
                      detail::tags::void_result const &,
                      detail::tags::nonzero_arg const &) {
    using detail::func_traits;
    using args_type = typename func_traits<F>::args_type;

    enforce_unique_name(name);
    funcs_.insert(
        std::make_pair(name, [func, name](RPCLIB_MSGPACK::object const &args) {
            constexpr int args_count = std::tuple_size<args_type>::value;
            enforce_arg_count(name, args_count, args.via.array.size);
            args_type args_real;
            args.convert(args_real);
            detail::call(func, args_real);
            return rpc::detail::make_unique<RPCLIB_MSGPACK::object_handle>();
        }));
}

template <typename F>
void dispatcher::bind(std::string const &name, F func,
                      detail::tags::nonvoid_result const &,
                      detail::tags::zero_arg const &) {
    using detail::func_traits;

    enforce_unique_name(name);
    funcs_.insert(std::make_pair(name, [func,
                                        name](RPCLIB_MSGPACK::object const &args) {
        enforce_arg_count(name, 0, args.via.array.size);
        auto z = rpc::detail::make_unique<RPCLIB_MSGPACK::zone>();
        auto result = RPCLIB_MSGPACK::object(func(), *z);
        return rpc::detail::make_unique<RPCLIB_MSGPACK::object_handle>(result, std::move(z));
    }));
}

template <typename F>
void dispatcher::bind(std::string const &name, F func,
                      detail::tags::nonvoid_result const &,
                      detail::tags::nonzero_arg const &) {
    using detail::func_traits;
    using args_type = typename func_traits<F>::args_type;

    enforce_unique_name(name);
    funcs_.insert(std::make_pair(name, [func,
                                        name](RPCLIB_MSGPACK::object const &args) {
        constexpr int args_count = std::tuple_size<args_type>::value;
        enforce_arg_count(name, args_count, args.via.array.size);
        args_type args_real;
        args.convert(args_real);
        auto z = rpc::detail::make_unique<RPCLIB_MSGPACK::zone>();
        auto result = RPCLIB_MSGPACK::object(detail::call(func, args_real), *z);
        return rpc::detail::make_unique<RPCLIB_MSGPACK::object_handle>(result, std::move(z));
    }));
}

template<typename Arg> const char* ReturnTypeName()
{
    const std::string pattern="const char* rpc::detail::ReturnTypeName() [with";

    return __PRETTY_FUNCTION__+pattern.length();
}

template<typename Arg> void PrintType(const std::string& msg= std::string())
{
    if(msg.length() > 0)
    {
        printf("\"%s\"%s\n",msg.c_str(),ReturnTypeName<Arg>());
    }
    else
    {
        printf("%s\n",ReturnTypeName<Arg>());
    }
    
}
#define PRINTTYPE(x) ::rpc::detail::PrintType<x>(#x)

template <typename F>
    void dispatcher::bind(std::string const &name, F func,
              detail::tags::void_result const &,
              detail::tags::nonzero_arg const &,
              detail::tags::refs_args const&)
{
    using detail::func_traits;
    using args_type = typename func_traits<F>::args_type;
    using ref_args_type = typename func_traits<F>::refs_args_type;
    using producer = RefArgsProducer<args_type>;
    using retriever = producer::template RefArgsPointer<ref_args_type>;
    using ref_args_type_tuple = typename retriever::tuple_type;
    using ref_args_type_tuple_seq = typename retriever::sequence_type;
    enforce_unique_name(name);
    funcs_.insert(
        std::make_pair(name, [func, name](RPCLIB_MSGPACK::object const &args) {
            constexpr int args_count = std::tuple_size<args_type>::value;
            enforce_arg_count(name, args_count, args.via.array.size);
            args_type args_real;
            args.convert(args_real);
            detail::call(func, args_real);
            auto z = rpc::detail::make_unique<RPCLIB_MSGPACK::zone>();
            auto result = RPCLIB_MSGPACK::object( producer::template ReprojectTuple<ref_args_type_tuple,ref_args_type_tuple_seq>::Reproject(std::move(args_real))   , *z);
            return rpc::detail::make_unique<RPCLIB_MSGPACK::object_handle>(result, std::move(z));
        }));
}

template <typename F>
    void dispatcher::bind(std::string const &/*name*/, F /*func*/,
              detail::tags::nonvoid_result const &,
              detail::tags::nonzero_arg const &,
              detail::tags::refs_args const&)
{
    //bind(name,func,detail::tags::nonvoid_result{},detail::tags::nonzero_arg{});
}

}
}
