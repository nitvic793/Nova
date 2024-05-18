#pragma once

#include <stdint.h>
#include <Lib/StringHash.h>
#include <Store/Store.h>
#include <Math/Math.h>

namespace nv::sim::city
{
    // Residential, Industrial, Commercial, Office
    struct ResidentialID    : public UInt64Property {};
    struct IndustrialID     : public UInt64Property {};
    struct OfficeID         : public UInt64Property {};
    struct CommercialID     : public UInt64Property {};    
}