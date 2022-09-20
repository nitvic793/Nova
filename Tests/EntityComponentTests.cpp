
#include "pch.h"
#include "TestCommon.h"

#include <Engine/EntityComponent.h>

namespace nv::tests
{
    using namespace ecs;

    struct AComponent : public IComponent
    {
        float mSpeed;
        float mMuliplier;
    };

    struct BComponent : public IComponent
    {
        std::string mString;
        uint64_t    mCount;
    };

    TEST_F(EntityComponentTests, EntityCreateTest)
    {
        auto e1 = gEntityManager.Create();
        auto e2 = gEntityManager.Create();

        auto entity1 = gEntityManager.GetEntity(e1);
        auto entity2 = gEntityManager.GetEntity(e2);

        EXPECT_TRUE(entity1 != nullptr);
        EXPECT_TRUE(entity2 != nullptr);
    }

    TEST_F(EntityComponentTests, EntityAddComponent)
    {
        auto e1 = gEntityManager.Create();
        auto entity1 = gEntityManager.GetEntity(e1);
        EXPECT_TRUE(entity1 != nullptr);

        entity1->Add<AComponent>();
        entity1->Add<BComponent>();

        EXPECT_EQ(entity1->mComponents.size(), 2);
    }

    TEST_F(EntityComponentTests, EntityGetComponent)
    {
        auto e1 = gEntityManager.Create();
        auto e2 = gEntityManager.Create();
        auto entity1 = gEntityManager.GetEntity(e1);
        auto entity2 = gEntityManager.GetEntity(e2);
        EXPECT_TRUE(entity1 != nullptr);

        entity1->Add<AComponent>();
        AComponent* comp = entity1->Get<AComponent>();
        EXPECT_TRUE(comp != nullptr);
        comp->mSpeed = 1.f;
        comp->mMuliplier = 2.f;

        auto sameComp = entity1->Get<AComponent>();

        EXPECT_NEAR(sameComp->mSpeed, 1.f, FLT_EPSILON); 
        EXPECT_NEAR(sameComp->mMuliplier, 2.f, FLT_EPSILON);

        auto bComp = entity2->Add<BComponent>();
        bComp->mString = "Test";
        bComp->mCount = UINT64_MAX;
    }

    TEST_F(EntityComponentTests, EntityMultipleTest)
    {
        constexpr uint32_t TEST_COUNT = 100000;
        static Handle<Entity> entities[TEST_COUNT];

        for (auto& e : entities)
        {
            e = gEntityManager.Create();
        }

        for (const auto& e : entities)
        {
            auto entity = gEntityManager.GetEntity(e);
            entity->Add<AComponent>();
        }

        auto components = gComponentManager.GetComponents<AComponent>();
        for (auto& comp : components)
        {
            comp.mMuliplier = 2.f;
            comp.mSpeed = 100.f;
        }

        for (const auto& e : entities)
        {
            auto entity = gEntityManager.GetEntity(e);
            auto comp = entity->Get<AComponent>();
            EXPECT_NEAR(comp->mSpeed, 100.f, FLT_EPSILON);
            EXPECT_NEAR(comp->mMuliplier, 2.f, FLT_EPSILON);
        }
    }
}