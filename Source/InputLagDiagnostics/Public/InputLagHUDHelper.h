#pragma once

#include "Core.h"
#include "GameFramework/HUD.h"

/**
 * Helper class to access protected Canvas member from AHUD
 */
class FInputLagHUDHelper
{
public:
	static UCanvas* GetHUDCanvas(AHUD* HUD)
	{
		// Use reinterpret_cast to access the protected Canvas member
		// This is safe because we know the memory layout of AHUD
		struct HUDWithPublicCanvas : public AHUD
		{
			using AHUD::Canvas;
		};
		
		return static_cast<HUDWithPublicCanvas*>(HUD)->Canvas;
	}
};
