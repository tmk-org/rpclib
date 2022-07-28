#include "gtest/gtest.h"

#include "rpc/client.h"
#include "rpc/server.h"
#include "rpc/rpc_error.h"
#include "testutils.h"

#include <sstream>
#include <chrono>
#include <thread>

using namespace rpc::testutils;

void complexFunc(std::string& ss,int p,long& z,float& ff)
{
    z=20*p;
    ff = 1/(z*1.f);
    ss = "after call p="+std::to_string(p)+",z="+std::to_string(z)+",ff="+std::to_string(ff);
}

size_t complexFuncWithRefRetNonMember(int& p,double d,std::string& ret_str,const std::string& msg)
{
    p++;
    double temp = d*10.0*(p+3);
    ret_str = "ret str "+msg+" "+std::to_string(p)+" "+std::to_string(d);
    return (size_t) temp;
}

class client_test : public testing::Test {
public:
    client_test() : s("127.0.0.1", test_port), is_running_(false) {
        s.bind("dummy_void_zeroarg", [this]() { md.dummy_void_zeroarg(); });
        s.bind("dummy_void_singlearg",
               [this](int x) { md.dummy_void_singlearg(x); });
        s.bind("dummy_void_multiarg",
               [this](int x, int y) { md.dummy_void_multiarg(x, y); });
        s.bind("large_return", [](std::size_t bytes){ get_blob(bytes); });
        s.bind("sleep", [](uint64_t ms) {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        });
        s.bind(
            "dummy_void_single_refarg",
            [this](int& i)
            {
                md.dummy_void_single_refarg(i);
            });
        s.bind(
            "dummy_void_double_refarg",
            [this](int i,int& i1)
            {
                md.dummy_void_double_refarg(i,i1);
            });
        s.bind("ref_complex_arg_func",
                [](std::string& ss,int p,long& z,float& ff)
                {
                    complexFunc(ss,p,z,ff);
                });
        s.bind("complexFuncWithRet",complexFuncWithRefRetNonMember);
        s.async_run();
    }

    void ref_arg_func()
    {
        rpc::client client("127.0.0.1", test_port);
        int k = 5;
        int k_before = k;
        //int& s = k;
        client.call<int&>("dummy_void_single_refarg", k);
        EXPECT_NE(k,k_before);
        
    }

    void ref_double_arg_func()
    {
        rpc::client client("127.0.0.1", test_port);
        int i = 5;
        int s = 10;
        int s_before = s;
        client.call<int,int&>("dummy_void_double_refarg",i ,std::forward<int&>(s));
        EXPECT_NE(s,s_before);
    }


    void ref_complex_arg_func()
    {
        rpc::client client("127.0.0.1", test_port);
        int i = 5;
        long s = 0;
        long s_before = s;
        float ff=3.14159f;
        float ff_before = ff;
        std::string ss="before call";
        std::string ss_before = ss;
        client.call<std::string&,int,long&,float&>("ref_complex_arg_func",ss,i,s,ff);
        EXPECT_TRUE((ss!= ss_before) && (s!=s_before) && (ff!=ff_before));
    }

    size_t complexFuncWithRefRet(int& p,double d,std::string& ret_str,const std::string& msg)
    {
        rpc::client client("127.0.0.1", test_port);
        int i = 5;
        
        double ff=3.14159;
        double ff_before = ff;
        std::string ss="before call";
        std::string ss_before = ss;
        size_t res=client.call<int&,double,std::string& ,const std::string&>("ref_complex_arg_func",i,ff_before,ss_before,std::string("msg")).as<size_t>();
        return 0;
    }
protected:
    static RPCLIB_CONSTEXPR uint16_t test_port = rpc::constants::DEFAULT_PORT;
    MockDummy md;
    rpc::server s;
    std::atomic_bool is_running_;
};

TEST_F(client_test, instantiation) {
    rpc::client client("127.0.0.1", test_port);
}

TEST_F(client_test, call) {
    EXPECT_CALL(md, dummy_void_zeroarg());
    EXPECT_CALL(md, dummy_void_singlearg(5));
    EXPECT_CALL(md, dummy_void_multiarg(5, 6));
    rpc::client client("127.0.0.1", test_port);
    client.call("dummy_void_zeroarg");
    client.call("dummy_void_singlearg", 5);
    client.call("dummy_void_multiarg", 5, 6);
}

TEST_F(client_test, notification) {
    EXPECT_CALL(md, dummy_void_zeroarg());
    rpc::client client("127.0.0.1", test_port);
    client.send("dummy_void_zeroarg");
    client.wait_all_responses();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_F(client_test,double_ref_call){
    ref_arg_func();
    ref_double_arg_func();
    ref_complex_arg_func();
}

TEST_F(client_test, large_return) {
    rpc::client client("127.0.0.1", test_port);
    std::size_t blob_size = 2 << 10 << 10;
    for (int i = 0; i < 4; ++i) {
        client.call("large_return", blob_size);
        blob_size *= 2;
    }
    // no crash is enough
}

TEST_F(client_test, timeout_setting_works) {
    rpc::client client("127.0.0.1", test_port);
    EXPECT_FALSE(client.get_timeout());

    const uint64_t short_timeout = 50;
    client.set_timeout(short_timeout);

    EXPECT_EQ(*client.get_timeout(), short_timeout);
    EXPECT_THROW(client.call("sleep", short_timeout + 10), rpc::timeout);

    client.set_timeout(short_timeout * 2);
    EXPECT_EQ(*client.get_timeout(), short_timeout * 2);
    EXPECT_NO_THROW(client.call("sleep", short_timeout + 1));
}

TEST_F(client_test, timeout_right_msg) {
    rpc::client client("127.0.0.1", test_port);
    const uint64_t short_timeout = 50;
    try {
        client.set_timeout(short_timeout);
        client.call("sleep", short_timeout + 10);
        FAIL() << "There was no exception thrown.";
    } catch (rpc::timeout &t) {
        std::stringstream ss;
        ss 
          << "rpc::timeout: Timeout of " 
          << *client.get_timeout()
          << "ms while calling RPC function 'sleep'";
        EXPECT_TRUE(str_match(t.what(), ss.str()));
    }
}

TEST_F(client_test, timeout_clear) {
    rpc::client client("127.0.0.1", test_port);
    EXPECT_FALSE(client.get_timeout());
    client.set_timeout(50);
    EXPECT_EQ(50, *client.get_timeout());
    client.clear_timeout();
    EXPECT_FALSE(client.get_timeout());
}

// Only enable this test on linux
// It seems like the connection error is not detected on windows
TEST_F(client_test, bad_ip) {
    rpc::client client("127.0.0.2", test_port);
    client.set_timeout(1000);
#ifdef __linux__
    EXPECT_THROW(client.call("dummy_void_zeroarg"), rpc::system_error);
    // We expect a connection refused, not a timeout
#else
    EXPECT_ANY_THROW(client.call("dummy_void_zeroarg"));
    // throw is enough for windows
#endif
}

TEST(client_test2, timeout_while_connection) {
    rpc::client client("localhost", rpc::constants::DEFAULT_PORT);
    client.set_timeout(50);
#ifdef __linux__
    // this client never connects, so this tests the timout in wait_conn()
    // We expect a connection refused, not a timeout
    EXPECT_THROW(client.call("whatev"), rpc::system_error);
#else
    EXPECT_ANY_THROW(client.call("dummy_void_zeroarg"));
    // throw is enough for windows
#endif
}
