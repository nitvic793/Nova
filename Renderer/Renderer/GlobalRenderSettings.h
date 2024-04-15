#pragma once

namespace nv::graphics
{
	struct Settings
	{
		bool mbEnableVSync			= false;
		bool mbEnableRTShadows		= false;
		bool mbEnableRTDiffuseGI	= false;
	};

	extern Settings gRenderSettings;
}