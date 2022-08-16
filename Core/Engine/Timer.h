#pragma once

#include <algorithm>

namespace nv
{
	class Timer
	{
	public:
		Timer()
		{
			if(!Instance)
				Instance = this;
		};

		void Start();
		void Tick();

		static const Timer* const GetInstance()
		{
			return Instance;
		}

		float DeltaTime = 0.f;
		float TotalTime = 0.f;

	private:


		uint64_t startTime = 0;
		uint64_t currentTime = 0;
		uint64_t previousTime = 0;
		uint64_t frameCounter = 0;

		double perfCounterSeconds = 0;

		static Timer* Instance;
	};
}