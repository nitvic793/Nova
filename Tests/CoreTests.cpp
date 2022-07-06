#include "pch.h"
#include <Lib/StringHash.h>
#include <Engine/System.h>

class CoreTests : public ::testing::Test
{
public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    static void SetUpTestSuite()
    {
        nv::InitContext(nullptr);
    }

    static void TearDownTestSuite() 
    {
        nv::DestroyContext();
    }
};

TEST_F(CoreTests, BasicMemTest)
{
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
}

TEST_F(CoreTests, VectorInit)
{
    nv::Vector v({ 1,2,3 });
    EXPECT_EQ(v.Size(), 3);
}

TEST_F(CoreTests, VectorSliceTest)
{
    nv::Vector<int> v({ 1,2,3,4,5,6,7,8 });

    auto slice = v.Slice(5, 7);
    for (auto& item : slice)
    {
        item++;
    }

    EXPECT_EQ(v[5], 7);
    EXPECT_EQ(v[6], 8);
    EXPECT_EQ(v[7], 9);
}

class IBase
{
public:
    virtual void Test(int) = 0;
    virtual int GetTest() = 0;
    virtual ~IBase() {}
};

class Child : public IBase
{
public:
    Child(int t) : mTest(t) {}
    Child() : mTest(0) {}
    void Test(int val) override { mTest = val; }
    int GetTest() override { return mTest; }

private:
    int mTest;
};

TEST_F(CoreTests, PoolTest)
{
    using namespace nv;

    Handle<IBase> handle;
    handle.mIndex = 1;
    Pool<IBase, Child> pool;
    Child test;
    test.Test(100);
    handle = pool.Create(1);
    auto handle2 = pool.Create(2);
    auto handle3 = pool.Insert(test);
    auto data = pool.Get(handle);

    auto data2 = pool.Get(handle2);
    EXPECT_EQ(data2->GetTest(), 2);

    pool.Remove(handle);
    const bool r = pool.IsValid(handle);
    EXPECT_FALSE(r);

    const bool r2 = pool.IsValid(handle2);
    EXPECT_TRUE(r2);

    auto data3 = pool.GetAsDerived(handle2);
    EXPECT_EQ(data3->GetTest(), 2);

    Child& data4 = *pool.GetAsDerived(handle3);
    EXPECT_EQ(data4.GetTest(), 100);

    data4.Test(101);
}

struct IComponent {};

struct TestComponent : public IComponent
{
    float mSpeed;
    float mMuliplier;
};

TEST_F(CoreTests, PoolSpanTest)
{
    using namespace nv;

    Pool<IComponent, TestComponent> comPool;
    auto h1 = comPool.Insert({ .mSpeed = 1.5f, .mMuliplier = 1.f });
    auto h2 = comPool.Create(TestComponent{ .mSpeed = 2.5f, .mMuliplier = 2.f });
    auto comSpan = comPool.Span();

    for (TestComponent& comp : comSpan)
    {
        comp.mMuliplier *= 2.f;
    }

    for (auto i = 0llu; i < comSpan.Size(); ++i)
    {
        comSpan[i].mMuliplier *= 2.f;
    }

    EXPECT_FLOAT_EQ(comPool.GetAsDerived(h1)->mMuliplier, 4.f);
    EXPECT_FLOAT_EQ(comPool.GetAsDerived(h2)->mMuliplier, 8.f);

}

TEST_F(CoreTests, StringHashTest)
{
    using namespace nv;
    constexpr nv::StringID check = "GET"_hash;
    static_assert(check == 2531704439, "bad hash value");
}

TEST_F(CoreTests, SystemManagerCreate)
{
    using namespace nv;
    class TestSystem : public ISystem
    {
    public:
        float mSpeed = 0.f;
    };

    SystemManager sysMan;

    auto testSystem = (TestSystem*)sysMan.CreateSystem<TestSystem>();
    EXPECT_FLOAT_EQ(testSystem->mSpeed, 0.f);

    testSystem->mSpeed = 1.f;

    auto testSystemRef = sysMan.GetSystem<TestSystem>();
    EXPECT_FLOAT_EQ(testSystemRef->mSpeed, 1.f);

    testSystemRef->mSpeed = 2.f;

    auto testSystemRef2 = (TestSystem*)sysMan.GetSystem(TypeNameID<TestSystem>());
    EXPECT_FLOAT_EQ(testSystemRef->mSpeed, 2.f);
}