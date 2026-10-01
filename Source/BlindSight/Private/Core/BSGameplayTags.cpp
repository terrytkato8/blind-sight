#include "Core/BSGameplayTags.h"

namespace BSTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Hunter,   "Role.Hunter",  "Player is the Hunter this round");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_Hider,    "Role.Hider",   "Player is a Hider this round");

	UE_DEFINE_GAMEPLAY_TAG(State_Eliminated,      "State.Eliminated");
	UE_DEFINE_GAMEPLAY_TAG(State_Sprinting,       "State.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(State_Crouching,       "State.Crouching");
	UE_DEFINE_GAMEPLAY_TAG(State_HeadStartFrozen, "State.HeadStartFrozen");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Fire,     "Ability.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Throw,    "Ability.Throw");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pulse,    "Ability.Pulse");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Interact, "Ability.Interact");

	UE_DEFINE_GAMEPLAY_TAG(Sound_Footstep_Crouch, "Sound.Footstep.Crouch");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Footstep_Walk,   "Sound.Footstep.Walk");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Footstep_Sprint, "Sound.Footstep.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Pickup,          "Sound.Pickup");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Throw_Release,   "Sound.Throw.Release");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Throw_Impact,    "Sound.Throw.Impact");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Door,            "Sound.Door");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Gunshot,         "Sound.Gunshot");
	UE_DEFINE_GAMEPLAY_TAG(Sound_Pulse,           "Sound.Pulse");
}
