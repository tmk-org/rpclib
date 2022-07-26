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

double dummy_int__double_refarg(int& p) 
{ 
    p++; 
    return p*1.5;
}

int dummy_multi_arg_wref(int& ,int ,std::string&,float,void*&)
{
    return 1;
}

void dummy_void_double_args_ref(int,int& n)
{
    n *= 10;
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
        dispatcher.bind("dummy_void_singlearg", &dummy_void_refarg);
        raw_call(raw_msg);
    }

    void ref_double_arg_func()
    {
        const unsigned char raw_msg[] = "\x94\x00\x00\xb3\x64\x75\x6d\x6d\x79\x5f"
                                "\x76\x6f\x69\x64\x5f\x6d\x75\x6c\x74\x69"
                                "\x61\x72\x67\x92\xcd\x01\x6b\x0c";
        dispatcher.bind("dummy_void_multiarg", &dummy_void_double_args_ref);
        raw_call(raw_msg);
        
    }
protected:
    rpc::detail::dispatcher dispatcher;
};

class dispatch_test : public binding_test {};

// The following raw messages were created with the python msgpack
// library from hand-crafted tuples of msgpack-rpc calls.

TEST_F(binding_test, freefunc_void_zeroarg) {
    const unsigned char raw_msg[] = "\x94\x00\x00\xb2\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x7a\x65\x72\x6f\x61"
                                    "\x72\x67\x90";
    dispatcher.bind("dummy_void_zeroarg", &dummy_void_zeroarg);
    raw_call(raw_msg);
    EXPECT_TRUE(g_dummy_void_zeroarg_called);
}

TEST_F(binding_test, freefunc_void_singlearg) {
    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                    "\x65\x61\x72\x67\x91\x2a";
    dispatcher.bind("dummy_void_singlearg", &dummy_void_singlearg);
    raw_call(raw_msg);
    EXPECT_TRUE(g_dummy_void_singlearg_called);
}







TEST_F(binding_test, freefunc_void_single_ref_arg) {
    //using func_args_types1 = rpc::detail::func_traits<decltype(&dummy_int_refarg)>::refs_args_type;
    //using func_args_types2 = rpc::detail::func_traits<decltype(&dummy_void_refarg)>::refs_args_type;
//    using func_args_types3_raw = rpc::detail::func_traits<decltype(& dummy_multi_arg_wref)>::args_type;
//    using func_args_types3 = rpc::detail::func_traits<decltype(& dummy_multi_arg_wref)>::refs_args_type;
    //using func_args_types4 = rpc::detail::func_traits<decltype(&dummy_void_multiarg)>::refs_args_type;
//
    //rpc::detail::PrintType<func_args_types1>();
    //rpc::detail::PrintType<func_args_types2>();
//    rpc::detail::PrintType<func_args_types3>();
//    rpc::detail::PrintType<func_args_types3_raw>();
//    rpc::detail::PrintType<rpc::detail::RefArgsProducer<func_args_types3_raw>::template RefArgsPointer<func_args_types3>::tuple_type>();
//    rpc::detail::PrintType<rpc::detail::RefArgsProducer<func_args_types3_raw>::template RefArgsPointer<func_args_types3>::sequence_type>();
    //rpc::detail::PrintType<func_args_types4>();
    //rpc::detail::PrintType<rpc::detail::has_ref_args<decltype(&dummy_void_multiarg)> >();
    //rpc::detail::PrintType<rpc::detail::has_ref_args<decltype(&dummy_multi_arg_wref)> >();
    auto lamtest = [&](int k,int i)
    {
        printf("%d %d\n",k,i);
        return 0;
    };
    auto lamtest2 = [&](int k,int& i)
    {
        printf("%d %d\n",k,i);
        return 0;
    };
    auto lamtest3 = [&](int& k,int i)
    {
        printf("%d %d\n",k,i);
        return 0;
    };
    auto lamtest4=[](const std::string& p)
    {
        return p;
    };
    using functype = decltype(lamtest);
    using lam_args_type_traits  = rpc::detail::func_traits< functype >;
    using merged_args_types     = lam_args_type_traits::merged_args_type;
    using result_type           = lam_args_type_traits::result_type   ;
    using refs_args_type        = lam_args_type_traits::refs_args_type;
    using arg_count             = lam_args_type_traits::arg_count     ;
    using args_type             = lam_args_type_traits::args_type     ;
    
    PRINTTYPE(functype);  
    PRINTTYPE(lam_args_type_traits);  
    PRINTTYPE(merged_args_types   );  
    PRINTTYPE(result_type         );  
    PRINTTYPE(refs_args_type      );  
    PRINTTYPE(arg_count           );  
    PRINTTYPE(args_type           ); 
    PRINTTYPE(rpc::detail::has_ref_args< functype > ) ;
    PRINTTYPE(rpc::detail::has_ref_args< functype >);

    PRINTTYPE(rpc::detail::func_traits< decltype(lamtest2) >::refs_args_type);
    PRINTTYPE(rpc::detail::func_traits< decltype(lamtest2) >::result_type);
    PRINTTYPE(rpc::detail::has_ref_args< decltype(lamtest2) >);

    PRINTTYPE(rpc::detail::func_traits< decltype(lamtest3) >::refs_args_type);
    PRINTTYPE(rpc::detail::func_traits< decltype(lamtest3) >::result_type);
    PRINTTYPE(rpc::detail::has_ref_args< decltype(lamtest3) >);

    PRINTTYPE(rpc::detail::func_traits< decltype(lamtest4) >::refs_args_type);
    PRINTTYPE(rpc::detail::func_traits< decltype(lamtest4) >::result_type);
    PRINTTYPE(rpc::detail::has_ref_args< decltype(lamtest4) >);
    using lam4types=rpc::detail::ReferenceTupleElementHandlerImplRoot<std::integral_constant<size_t,1>,const std::string&,void>::type;
    using lam4argtype = const std::string&;
    using lam4typescond_1  = std::conditional_t< 
                                    !rpc::detail::is_tuple<lam4argtype>::value && 
                                    (std::is_reference_v<lam4argtype> && !std::is_const_v<std::remove_reference_t<lam4argtype>>),
                                    std::true_type,std::false_type >;
    PRINTTYPE(lam4typescond_1);
    EXPECT_NO_THROW(
        ref_arg_func();
        ref_double_arg_func();
    );
    
}


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

TEST_F(dispatch_test, argcount_verified_void_nonzero_arg_too_few) {
    // raw_msg contains a call to dummy_void_singlearg but zero arguments
    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                    "\x65\x61\x72\x67\x90";
    dispatcher.bind("dummy_void_singlearg", &dummy_void_singlearg);
    EXPECT_NO_THROW(raw_call(raw_msg));
    EXPECT_FALSE(g_dummy_void_singlearg_called);
}

TEST_F(dispatch_test, argcount_verified_void_nonzero_arg_too_many) {
    // raw_msg contains a call to dummy_void_singlearg but with two
    const unsigned char raw_msg[] = "\x94\x00\x00\xb4\x64\x75\x6d\x6d\x79\x5f"
                                    "\x76\x6f\x69\x64\x5f\x73\x69\x6e\x67\x6c"
                                    "\x65\x61\x72\x67\x92\x2a\x2b";
    dispatcher.bind("dummy_void_singlearg", &dummy_void_singlearg);
    EXPECT_NO_THROW(raw_call(raw_msg));
    EXPECT_FALSE(g_dummy_void_singlearg_called);
}

TEST_F(dispatch_test, unbound_func_error_response) {
    dispatcher.bind("foo", &dummy_void_singlearg);
    auto msg = make_unpacked(0, 0, "bar", RPCLIB_MSGPACK::type::nil_t());
    auto response = dispatcher.dispatch(msg.get());
    EXPECT_TRUE(response.get_error() !=
                std::shared_ptr<RPCLIB_MSGPACK::object_handle>());
}

TEST_F(dispatch_test, bad_format_msgpack_returns_empty) {
    auto msg = make_unpacked(1, 2, 3, 4, 5); // 5 items is breaking the protocol
    auto response = dispatcher.dispatch(msg.get());
    EXPECT_TRUE(response.is_empty());
}

TEST_F(dispatch_test, unique_names_zeroarg) {
    dispatcher.bind("foo", &dummy_void_zeroarg);
    EXPECT_THROW(dispatcher.bind("foo", &dummy_void_zeroarg), std::logic_error);
}

TEST_F(dispatch_test, unique_names_singlearg) {
    dispatcher.bind("foo", &dummy_void_singlearg);
    EXPECT_THROW(dispatcher.bind("foo", &dummy_void_singlearg), std::logic_error);
}

TEST_F(dispatch_test, unique_names_multiarg) {
    dispatcher.bind("foo", &dummy_void_multiarg);
    EXPECT_THROW(dispatcher.bind("foo", &dummy_void_multiarg), std::logic_error);
}

TEST_F(dispatch_test,non_void_ref_arg_test)
{
    using F = decltype(&dummy_int__double_refarg);
    PRINTTYPE(F);
    using rpc::detail::func_traits;
    using args_type = typename func_traits<F>::args_type;
    using ref_args_type = typename func_traits<F>::refs_args_type;
    using producer = rpc::detail::RefArgsProducer<args_type>;
    using retriever = producer::template RefArgsPointer<ref_args_type>;
    using ref_args_type_tuple = typename retriever::tuple_type;
    using ref_args_type_tuple_seq = typename retriever::sequence_type;
    PRINTTYPE(args_type);
    PRINTTYPE(ref_args_type);
    PRINTTYPE(producer);
    PRINTTYPE(retriever);
    PRINTTYPE(ref_args_type_tuple);
    PRINTTYPE(ref_args_type_tuple_seq);
    RPCLIB_MSGPACK::object args_object(args_type(2));

    args_type rev_args{};
    args_object.convert(rev_args);
    EXPECT_EQ(std::get<0>(rev_args),2);
    //args_object.convert()
    dispatcher.bind("dummy_int_refarg",&dummy_int__double_refarg);
}