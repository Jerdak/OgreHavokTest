#include "HKFactory.h"
#include "GHavok.h"

HKFactory::HKFactory(const HKFactoryDescrip &d):HKObject(d){
	_descrip = d;
}

HKFactory::~HKFactory(){
}

void HKFactory::AddObject(HKObject *obj){
	_vObj.push_back(obj);
}

void HKFactory::RemoveAllObjects(){
	_vObj.clear();
}

void HKFactory::Step(double dt){
	//Loop through all of our objects and update 
	std::vector<HKObject*>::iterator iter;
	for(iter = _vObj.begin() ;iter != _vObj.end(); iter++){
		(*iter)->Step(dt);
	}
}