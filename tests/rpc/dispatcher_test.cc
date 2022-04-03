#include <functional>

#include "gmock/gmock.h"

#include "rpc/dispatcher.h"
#include "testutils.h"

using namespace rpc::testutils;

// Unit tests for binding functions to names.
// A couple of these tests exist to check if the code compiles correctly,
// obviously that is not in the scope of normal unit testing.

// Yes, ugly global variables right away. We have to test for free functions
// somehow though :(
bool g_dummy_void_zeroarg_called;
bool g_dummy_void_singlearg_called;
bool g_dummy_void_multiarg_called;
void dummy_void_zeroarg() { g_dummy_void_zeroarg_called = true; }
void dummy_void_singlearg(int) { g_dummy_void_singlearg_called = true; }
void dummy_void_multiarg(int, int) { g_dummy_void_multiarg_called = true; }
void dummy_void_refarg(int& p) 
{ 
    p++; 
}

int dummy_int_refarg(int& p) 
{ 
    p++; 
    return p;
}

int dummy_multi_arg_wref(int& ,int ,std::string&,float,void*&)
{
    return 1;
}

class binding_test : public testing::Test {
public:
    binding_test() : dispatcher() {
        g_dummy_void_zeroarg_called = false;
        g_dummy_void_singlearg_called = false;
        g_dummy_void_multiarg_called = false;
    }

    template <typename A> void raw_call(A &&msg_array) {
        RPCLIB_MSGPACK::sbuffer msg;
        msg.write(reinterpret_cast<const char *>(msg_array), sizeof(msg_array));
        dispatcher.dispatch(msg);
    }
    void ref_arg_func()
    {
        const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                        "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                        "\x65\x61\x72\x67\x91\x2a";
        dispatcher.bind("dummy_void_refarg", &dummy_void_refarg);
        raw_call(raw_msg);
    }

protected:
    rpc::detail::dispatcher dispatcher;
};

class dispatch_test : public binding_test {};

// The following raw messages were created with the python msgpack
// library from hand-crafted tuples of msgpack-rpc calls.
/*
TEST_F(binding_test, freefunc_void_zeroarg) {
    const unsigned char raw_msg[] = "\x94\x00\x00\xb2\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x7a\x65\x72\x6f\x61"
                                    "\x72\x67\x90";
    dispatcher.bind("dummy_void_zeroarg", &dummy_void_zeroarg);
    raw_call(raw_msg);
    EXPECT_TRUE(g_dummy_void_zeroarg_called);
}*/
/*
TEST_F(binding_test, freefunc_void_singlearg) {
    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                    "\x65\x61\x72\x67\x91\x2a";
    dispatcher.bind("dummy_void_singlearg", &dummy_void_singlearg);
    raw_call(raw_msg);
    EXPECT_TRUE(g_dummy_void_singlearg_called);
}*/

template<size_t Index,typename T> struct element_holder;

template<size_t Index,typename T> struct element_holder<Index,T&>:std::integral_constant<size_t,Index>
{};

template<typename Head,typename... Rest> struct ReferenceTupleElementHandler;

template<   typename T,
            typename T1,
            typename... T2
            > struct ReferenceTupleElementHandlerImplRoot;

template<typename T1,typename...T2> struct TupleVoidRemover;

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t<   (std::tuple_size_v< std::tuple<T1,T2...> > == 1) 
                                                    && (!std::is_void_v<T1>)
                                                > 
                            >
{
    using type=std::tuple<T1>;
};

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t<   (std::tuple_size_v< std::tuple<T1,T2...> > == 1) 
                                                    && (std::is_void_v<T1>)
                                                > 
                            >
{
    using type=void;
};

template<typename T1,typename...T2,size_t...idxs> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::index_sequence<idxs...> 
                            >
{
    using type = std::tuple< std::tuple_element_t<idxs,std::tuple<T1,T2...> >... >;
};

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t< (std::tuple_size_v< std::tuple<T1,T2...> > > 1) 
                                                && std::is_void_v<
                                                                std::tuple_element_t< 
                                                                                    std::tuple_size_v< 
                                                                                                    std::tuple<T1,T2...> 
                                                                                                    > - 1, 
                                                                                    std::tuple<T1,T2...> 
                                                                                    > 
                                                                > 
                                                > 
                            >
{
    using tuple_type = std::tuple<T1,T2...>;
    using type= TupleVoidRemover<tuple_type,std::make_index_sequence< std::tuple_size_v< tuple_type > -1 > >::type;
};

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t< 
                                                (std::tuple_size_v< std::tuple<T1,T2...> > > 1) 
                                                &&  !std::is_void_v<
                                                                    std::tuple_element_t< 
                                                                                    std::tuple_size_v< std::tuple<T1,T2...> > - 1, 
                                                                                    std::tuple<T1,T2...> 
                                                                                        > 
                                                                    > 
                                                    
                                                > 
                            >
{
    using type=std::tuple<T1,T2...>;
};

template<typename Head,typename... Rest> struct ReferenceTupleElementHandler<std::tuple<Head,Rest...> >
{
    using preliminary_type = ReferenceTupleElementHandlerImplRoot<
                                                std::integral_constant<size_t ,1+sizeof...(Rest)>,
                                                std::tuple<Head,Rest...>,
                                                void
                                            >::type;
    using type = TupleVoidRemover<
                                preliminary_type,
                                void
                                >::type;
};

template<typename T> struct is_tuple : std::false_type{};
template<typename... Ts> struct is_tuple< std::tuple<Ts...> > : std::true_type{};

template<   size_t TLen,
            typename T1
            > struct ReferenceTupleElementHandlerImplRoot<std::integral_constant<size_t ,TLen>,T1,std::enable_if_t< !is_tuple<T1>::value && std::is_reference_v<T1> > >
{
    using type = std::tuple<element_holder<TLen-1,T1>>;
};

template<   size_t TLen,
            typename T1
            > struct ReferenceTupleElementHandlerImplRoot<std::integral_constant<size_t ,TLen>,T1,std::enable_if_t< !is_tuple<T1>::value && !std::is_reference_v<T1> > >
{
    using type = std::tuple<void>;
};


template<   size_t TLen,
            typename T1,
            typename... Rest
            > struct ReferenceTupleElementHandlerImplRoot<  std::integral_constant<size_t ,TLen>,
                                                            std::tuple<T1,Rest...>,
                                                            std::enable_if_t< sizeof...(Rest)==0 > >
{
    using type = ReferenceTupleElementHandlerImplRoot< std::integral_constant<size_t ,TLen>,T1,void >::type;
};

template<   size_t TLen,
            typename T1,
            typename... Rest
            > struct ReferenceTupleElementHandlerImplRoot<  std::integral_constant<size_t ,TLen>,
                                                            std::tuple<T1,Rest...>,
                                                            std::enable_if_t< std::is_reference_v<T1> && (sizeof...(Rest) > 0) > >
{
    using additional_type=ReferenceTupleElementHandlerImplRoot<   std::integral_constant<size_t , TLen>,
                                                                    std::tuple<Rest...>,
                                                                    void >::type;
    using this_element_type = element_holder<TLen - 1 - sizeof...(Rest),T1>;                                                                    
    using type = decltype(std::tuple_cat(std::declval<std::tuple<this_element_type>>(),std::declval<additional_type>()));// std::tuple<this_element_type,additional_type>  ;
};

template<   size_t TLen,
            typename T1,
            typename... Rest
            > struct ReferenceTupleElementHandlerImplRoot<  std::integral_constant<size_t ,TLen>,
                                                            std::tuple<T1,Rest...>,
                                                            std::enable_if_t< !std::is_reference_v<T1>  && (sizeof...(Rest) > 0) > >
{
//    using type = element_holder<TLen - 1 - sizeof...(Rest),T1>;
    using type = ReferenceTupleElementHandlerImplRoot<   std::integral_constant<size_t , TLen>,
                                                                    std::tuple<Rest...>,
                                                                    void >::type  ;
};




template<typename T1,typename...T > struct ResultTraitsImpl;

template<typename Res> struct ResultTraitsImpl<Res ,std::enable_if_t< !std::is_reference_v<Res> > >
{
    typedef Res type;
};

template<typename... Args> struct pack_has_references
{
    static constexpr bool value {(std::is_lvalue_reference_v<Args> || ...)};
};


template<typename Res,typename... Args  > struct ResultTraitsImpl< Res ,std::enable_if_t< std::is_void_v<Res> && pack_has_references<Args...>::value >,Args...  >
{
    typedef std::tuple<Args...>  type;
};

template<typename Res,typename... Args  > struct ResultTraitsImpl< Res ,std::enable_if_t<  !pack_has_references<Args...>::value >,Args...  >
{
    typedef Res  type;
};

template<typename Res,typename... Args  > struct ResultTraitsImpl< Res ,std::enable_if_t< !std::is_void_v<Res> && pack_has_references<Args...>::value >,Args...  >
{
    typedef std::tuple<Res,Args...>  type;
};

template<typename T,typename...T1> struct ResultTraits : ResultTraitsImpl<T,void,T1...>
{

};

template<typename Arg> void PrintType()
{
    printf("%s\n",__PRETTY_FUNCTION__);
}


template<typename T> struct func_traits2;

template<typename Res,typename...Args> struct func_traits2<Res(*)(Args...)> : ResultTraits<Res,Args...>
{

};


TEST_F(binding_test, freefunc_void_single_ref_arg) {
    using func_args_types1 = func_traits2<decltype(&dummy_int_refarg)>::type;
    using func_args_types2 = func_traits2<decltype(&dummy_void_refarg)>::type;
    using func_args_types3 = func_traits2<decltype(& dummy_multi_arg_wref)>::type;
 
    PrintType<func_args_types1>();
    PrintType< ReferenceTupleElementHandler< func_args_types1 >::type>();

    PrintType<func_args_types2>();
    PrintType< ReferenceTupleElementHandler< func_args_types2 >::type>();

    PrintType<func_args_types3>();
    PrintType< ReferenceTupleElementHandler< func_args_types3 >::type>();

    EXPECT_NO_THROW(
        ref_arg_func();
    );
    
}

/*
TEST_F(binding_test, freefunc_void_multiarg) {
    const unsigned char raw_msg[] = "\x94\x00\x00\xb3\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x6d\x75\x6c\x74\x69"
                                    "\x61\x72\x67\x92\xcd\x01\x6b\x0c";
    dispatcher.bind("dummy_void_multiarg", &dummy_void_multiarg);
    raw_call(raw_msg);
    EXPECT_TRUE(g_dummy_void_multiarg_called);
}

TEST_F(binding_test, memfunc_void_zeroarg) {
    MockDummy md;
    EXPECT_CALL(md, dummy_void_zeroarg());
    const unsigned char raw_msg[] = "\x94\x00\x00\xb2\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x7a\x65\x72\x6f\x61"
                                    "\x72\x67\x90";
    dispatcher.bind("dummy_void_zeroarg", [&md]() { md.dummy_void_zeroarg(); });
    raw_call(raw_msg);
}

TEST_F(binding_test, memfunc_void_singlearg) {
    MockDummy md;
    EXPECT_CALL(md, dummy_void_singlearg(42));
    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                    "\x65\x61\x72\x67\x91\x2a";
    dispatcher.bind("dummy_void_singlearg",
                    [&md](int x) { md.dummy_void_singlearg(x); });
    raw_call(raw_msg);
}

TEST_F(binding_test, memfunc_void_multiarg) {
    MockDummy md;
    EXPECT_CALL(md, dummy_void_multiarg(363, 12));
    const unsigned char raw_msg[] = "\x94\x00\x00\xb3\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x6d\x75\x6c\x74\x69"
                                    "\x61\x72\x67\x92\xcd\x01\x6b\x0c";
    dispatcher.bind("dummy_void_multiarg",
                    [&md](int x, int y) { md.dummy_void_multiarg(x, y); });
    raw_call(raw_msg);
}

TEST_F(binding_test, stdfunc_void_zeroarg) {
    MockDummy md;
    EXPECT_CALL(md, dummy_void_zeroarg());
    const unsigned char raw_msg[] = "\x94\x00\x00\xb2\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x7a\x65\x72\x6f\x61"
                                    "\x72\x67\x90";
    dispatcher.bind(
        "dummy_void_zeroarg",
        std::function<void()>(std::bind(&IDummy::dummy_void_zeroarg, &md)));
    raw_call(raw_msg);
}

TEST_F(binding_test, stdfunc_void_singlearg) {
    using namespace std::placeholders;
    MockDummy md;
    EXPECT_CALL(md, dummy_void_singlearg(42));
    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                    "\x65\x61\x72\x67\x91\x2a";
    dispatcher.bind("dummy_void_singlearg",
                    std::function<void(int)>(
                        std::bind(&IDummy::dummy_void_singlearg, &md, _1)));
    raw_call(raw_msg);
}

TEST_F(binding_test, stdfunc_void_multiarg) {
    using namespace std::placeholders;
    MockDummy md;
    EXPECT_CALL(md, dummy_void_multiarg(363, 12));
    const unsigned char raw_msg[] = "\x94\x00\x00\xb3\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x6d\x75\x6c\x74\x69"
                                    "\x61\x72\x67\x92\xcd\x01\x6b\x0c";
    dispatcher.bind("dummy_void_multiarg",
                    std::function<void(int, int)>(
                        std::bind(&IDummy::dummy_void_multiarg, &md, _1, _2)));
    raw_call(raw_msg);
}
*/
//TEST_F(dispatch_test, argcount_verified_void_nonzero_arg_too_few) {
//    // raw_msg contains a call to dummy_void_singlearg but zero arguments
//    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
//                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
//                                    "\x65\x61\x72\x67\x90";
//    dispatcher.bind("dummy_void_singlearg", &dummy_void_singlearg);
//    EXPECT_NO_THROW(raw_call(raw_msg));
//    EXPECT_FALSE(g_dummy_void_singlearg_called);
//}
//
//TEST_F(dispatch_test, argcount_verified_void_nonzero_arg_too_many) {
//    // raw_msg contains a call to dummy_void_singlearg but with two
//    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
//                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
//                                    "\x65\x61\x72\x67\x92\x2a\x2b";
//    dispatcher.bind("dummy_void_singlearg", &dummy_void_singlearg);
//    EXPECT_NO_THROW(raw_call(raw_msg));
//    EXPECT_FALSE(g_dummy_void_singlearg_called);
//}
//
//TEST_F(dispatch_test, unbound_func_error_response) {
//    dispatcher.bind("foo", &dummy_void_singlearg);
//    auto msg = make_unpacked(0, 0, "bar", RPCLIB_MSGPACK::type::nil_t());
//    auto response = dispatcher.dispatch(msg.get());
//    EXPECT_TRUE(response.get_error() !=
//                std::shared_ptr<RPCLIB_MSGPACK::object_handle>());
//}
//
//TEST_F(dispatch_test, bad_format_msgpack_returns_empty) {
//    auto msg = make_unpacked(1, 2, 3, 4, 5); // 5 items is breaking the protocol
//    auto response = dispatcher.dispatch(msg.get());
//    EXPECT_TRUE(response.is_empty());
//}
//
//TEST_F(dispatch_test, unique_names_zeroarg) {
//    dispatcher.bind("foo", &dummy_void_zeroarg);
//    EXPECT_THROW(dispatcher.bind("foo", &dummy_void_zeroarg), std::logic_error);
//}
//
//TEST_F(dispatch_test, unique_names_singlearg) {
//    dispatcher.bind("foo", &dummy_void_singlearg);
//    EXPECT_THROW(dispatcher.bind("foo", &dummy_void_singlearg), std::logic_error);
//}
//
//TEST_F(dispatch_test, unique_names_multiarg) {
//    dispatcher.bind("foo", &dummy_void_multiarg);
//    EXPECT_THROW(dispatcher.bind("foo", &dummy_void_multiarg), std::logic_error);
//}

