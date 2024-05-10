#pragma once
#include <cstdint>

namespace nv::sim
{
    constexpr bool IsLeapYear(uint32_t year)
    {
        if (year % 4 == 0)
            return true;
        else
            return false;
    }

    constexpr uint32_t GetDaysInMonth(uint32_t month, uint32_t year)
    {
        uint32_t daysInMonth = 0;
        switch (month)
        {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            daysInMonth = 31;
        }

        switch (month)
        {
        case 2:
            if (IsLeapYear(year))
            {
                daysInMonth = 29;
            }
            else
            {
                daysInMonth = 28;
            }
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            daysInMonth = 30;
            break;
        }

        return daysInMonth;
    }

    struct SimTimer;

    constexpr void IncrementDate(SimTimer& timer, uint32_t byNum);

    struct SimTimer
    {
        uint32_t mDay;
        uint32_t mMonth;
        uint32_t mYear;

        constexpr void Tick()
        {
            IncrementDate(*this, 1);
        }
    };

    constexpr void IncrementDate(SimTimer& timer, uint32_t byNum)
    {
        while ((byNum + timer.mDay) > GetDaysInMonth(timer.mMonth, timer.mYear))
        {
            byNum -= GetDaysInMonth(timer.mMonth, timer.mYear);
            if (timer.mMonth == 12)
            {
                timer.mMonth = 0;
                timer.mYear++;
            }
            timer.mMonth++;
        }

        timer.mDay += byNum;
    }


}