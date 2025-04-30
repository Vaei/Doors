// Copyright (c) Jared Taylor


#include "DoorTags.h"


namespace FDoorTags
{
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail, "Door.Fail");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_CanDoorChangeToAnyState, "Door.Fail.CanDoorChangeToAnyState");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_OnCooldown, "Door.Fail.OnCooldown");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_InMotion, "Door.Fail.InMotion");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_ClientDoorSide, "Door.Fail.ClientDoorSide");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_CanChangeDoorState, "Door.Fail.CanChangeDoorState");
	
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_DoorNotValid, "Door.Fail.DoorNotValid");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_Locked, "Door.Fail.Locked");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_AlreadyClosed, "Door.Fail.AlreadyClosed");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_AlreadyOpen, "Door.Fail.AlreadyOpen");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_NoAccessFromFront, "Door.Fail.NoAccessFromFront");
	UE_DEFINE_GAMEPLAY_TAG(Door_Fail_NoAccessFromBack, "Door.Fail.NoAccessFromBack");
}