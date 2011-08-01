#pragma once
#include "OCLevelState.h"

#include <vcclr.h>
#using <mscorlib.dll>
#using <System.dll>


__gc class ManagedUtils {
public:
	ManagedUtils(){}
	~ManagedUtils(){}

	static gcroot<System::String *> LevelStateToString(LevelState *lvl){
		System::Text::StringBuilder *sb = __gc new System::Text::StringBuilder("");
		sb->Append(__box(lvl->roll.valueRadians())->ToString());
		sb->Append(" ");
		sb->Append(__box(lvl->pitch.valueRadians())->ToString());
		sb->Append(" ");
		sb->Append(__box(lvl->yaw.valueRadians())->ToString());
		sb->Append(" ");
		sb->Append(__box(lvl->x)->ToString();)
		sb->Append(" ");
		sb->Append(__box(lvl->y)->ToString());
		sb->Append(" ");
		sb->Append(__box(lvl->z)->ToString());
		return sb->ToString();
	}
};