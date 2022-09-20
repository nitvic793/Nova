#pragma once

#include "pch.h"
#include <Lib/StringHash.h>
#include <Engine/System.h>

namespace nv::tests
{
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

    class EntityComponentTests : public ::testing::Test
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
}