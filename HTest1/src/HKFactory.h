#pragma once
#include "HKObject.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "GHavok.h"

class HKFactoryDescrip : public HKObjectDescrip {
public:
	HKFactoryDescrip():HKObjectDescrip(){}
	~HKFactoryDescrip(){}


};
class HKFactory: public HKObject
{
public:
	HKFactory(const HKFactoryDescrip &d);
	~HKFactory(void);

	void AddObject(HKObject *obj);

	void RemoveAllObjects();
	void RemoveObject(char *name);
	void RemoveObject(int idx);

	void Step(double dt);

private:
	std::vector<HKObject*> _vObj;
	HKFactoryDescrip _descrip;
};
