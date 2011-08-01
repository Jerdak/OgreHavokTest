#pragma once
#include "HKObject.h"

class HKSpectator :
	public HKObject
{
public:
	HKSpectator(void);
	~HKSpectator(void);

	void Step(double dt);
};
