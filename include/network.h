#ifndef BULLSHITCORE_NETWORK
#define BULLSHITCORE_NETWORK

#include <stddef.h>
#include <stdint.h>
#include "nbt.h"

typedef _Bool Boolean;
#define true 1
#define false 0
typedef int8_t VarInt;
typedef int8_t VarLong;
typedef struct
{
	VarInt *length;
	const uint8_t *contents;
} String;
#define STRING_MAXSIZE 98301L
typedef struct
{
	size_t length;
	const uint32_t *contents;
} UnicodeString;
typedef NBT TextComponent;
typedef String JSONTextComponent;
#define JSONTEXTCOMPONENT_MAXSIZE 262144L
typedef String Identifier;
#define IDENTIFIER_MAXSIZE 32767
typedef struct
{
	uint8_t index;
	VarInt *type;
	void *value;
} EntityMetadata[];
typedef struct
{
	Boolean present;
	VarInt *item;
	uint8_t item_count;
	NBT NBT;
} Slot;
typedef uint64_t Position;
typedef uint8_t Angle;
typedef uint64_t UUID[2];
typedef struct
{
	VarInt *length;
	uint64_t data[];
} BitSet;
typedef uint8_t FixedBitSet[];
enum Connection_State
{
	Connection_State_Handshaking,
	Connection_State_Status,
	Connection_State_Login,
	Connection_State_Transfer,
	Connection_State_Configuration,
	Connection_State_Play
};
enum Packet_Handshaking_Client
{
	Packet_Handshaking_Client_Handshake
};
enum Packet_Status_Client
{
	Packet_Status_Client_Status_Request,
	Packet_Status_Client_Ping_Request
};
enum Packet_Status_Server
{
	Packet_Status_Server_Status_Response,
	Packet_Status_Server_Ping_Response
};
enum Packet_Login_Client
{
	Packet_Login_Client_Login_Start,
	Packet_Login_Client_Encryption_Response,
	Packet_Login_Client_Login_Plugin_Response,
	Packet_Login_Client_Login_Acknowledged,
	Packet_Login_Client_Cookie_Response
};
enum Packet_Login_Server
{
	Packet_Login_Server_Disconnect,
	Packet_Login_Server_Encryption_Request,
	Packet_Login_Server_Login_Success,
	Packet_Login_Server_Set_Compression,
	Packet_Login_Server_Login_Plugin_Request,
	Packet_Login_Server_Cookie_Request
};
enum Packet_Configuration_Client
{
	Packet_Configuration_Client_Client_Information,
	Packet_Configuration_Client_Cookie_Response,
	Packet_Configuration_Client_Plugin_Message,
	Packet_Configuration_Client_Finish_Configuration_Acknowledge,
	Packet_Configuration_Client_Keep_Alive,
	Packet_Configuration_Client_Pong, // pog
	Packet_Configuration_Client_Resource_Pack_Response,
	Packet_Configuration_Client_Known_Packs
};
enum Packet_Configuration_Server
{
	Packet_Configuration_Server_Cookie_Request,
	Packet_Configuration_Server_Plugin_Message,
	Packet_Configuration_Server_Disconnect,
	Packet_Configuration_Server_Finish_Configuration,
	Packet_Configuration_Server_Keep_Alive,
	Packet_Configuration_Server_Ping,
	Packet_Configuration_Server_Reset_Chat,
	Packet_Configuration_Server_Registry_Data,
	Packet_Configuration_Server_Remove_Resource_Pack,
	Packet_Configuration_Server_Add_Resource_Pack,
	Packet_Configuration_Server_Store_Cookie,
	Packet_Configuration_Server_Transfer,
	Packet_Configuration_Server_Feature_Flags,
	Packet_Configuration_Server_Update_Tags,
	Packet_Configuration_Server_Known_Packs,
	Packet_Configuration_Server_Custom_Report_Details,
	Packet_Configuration_Server_Server_Links
};
enum Packet_Play_Client
{
	Packet_Play_Client_Confirm_Teleportation,
	Packet_Play_Client_Query_Block_Entity_Tag,
	Packet_Play_Client_Change_Difficulty,
	Packet_Play_Client_Acknowledge_Message,
	Packet_Play_Client_Chat_Command,
	Packet_Play_Client_Signed_Chat_Command,
	Packet_Play_Client_Chat_Message,
	Packet_Play_Client_Player_Session,
	Packet_Play_Client_Chunk_Batch_Received,
	Packet_Play_Client_Client_Status,
	Packet_Play_Client_Tick_End,
	Packet_Play_Client_Client_Information,
	Packet_Play_Client_Command_Suggestions_Request,
	Packet_Play_Client_Acknowledge_Configuration,
	Packet_Play_Client_Click_Container_Button,
	Packet_Play_Client_Click_Container,
	Packet_Play_Client_Close_Container,
	Packet_Play_Client_Change_Container_Slot_State,
	Packet_Play_Client_Cookie_Response,
	Packet_Play_Client_Plugin_Message,
	Packet_Play_Client_Debug_Sample_Subscription,
	Packet_Play_Client_Edit_Book,
	Packet_Play_Client_Query_Entity_Tag,
	Packet_Play_Client_Interact,
	Packet_Play_Client_Jigsaw_Generate,
	Packet_Play_Client_Keep_Alive,
	Packet_Play_Client_Lock_Difficulty,
	Packet_Play_Client_Set_Player_Position,
	Packet_Play_Client_Set_Player_Position_And_Rotation,
	Packet_Play_Client_Set_Player_Rotation,
	Packet_Play_Client_Set_Player_On_Ground,
	Packet_Play_Client_Move_Vehicle,
	Packet_Play_Client_Paddle_Boat,
	Packet_Play_Client_Pick_Item,
	Packet_Play_Client_Ping_Request,
	Packet_Play_Client_Place_Recipe,
	Packet_Play_Client_Player_Abilities,
	Packet_Play_Client_Player_Action,
	Packet_Play_Client_Player_Command,
	Packet_Play_Client_Player_Input,
	Packet_Play_Client_Pong,
	Packet_Play_Client_Change_Recipe_Book_Settings,
	Packet_Play_Client_Set_Seen_Recipe,
	Packet_Play_Client_Rename_Item,
	Packet_Play_Client_Resource_Pack_Response,
	Packet_Play_Client_Seen_Advancements,
	Packet_Play_Client_Select_Trade,
	Packet_Play_Client_Set_Beacon_Effect,
	Packet_Play_Client_Set_Held_Item,
	Packet_Play_Client_Program_Command_Block,
	Packet_Play_Client_Program_Command_Block_Minecart,
	Packet_Play_Client_Set_Creative_Mode_Slot,
	Packet_Play_Client_Program_Jigsaw_Block,
	Packet_Play_Client_Program_Structure_Block,
	Packet_Play_Client_Update_Sign,
	Packet_Play_Client_Swing_Arm,
	Packet_Play_Client_Teleport_To_Entity,
	Packet_Play_Client_Use_Item_On,
	Packet_Play_Client_Use_Item
};
enum Packet_Play_Server
{
	Packet_Play_Server_Bundle_Delimiter,
	Packet_Play_Server_Spawn_Entity,
	Packet_Play_Server_Spawn_Experience_Orb,
	Packet_Play_Server_Entity_Animation,
	Packet_Play_Server_Award_Statistics,
	Packet_Play_Server_Acknowledge_Block_Change,
	Packet_Play_Server_Set_Block_Destroy_Stage,
	Packet_Play_Server_Block_Entity_Data,
	Packet_Play_Server_Block_Action,
	Packet_Play_Server_Block_Update,
	Packet_Play_Server_Boss_Bar,
	Packet_Play_Server_Change_Difficulty,
	Packet_Play_Server_Chunk_Batch_Finished,
	Packet_Play_Server_Chunk_Batch_Start,
	Packet_Play_Server_Chunk_Biomes,
	Packet_Play_Server_Reset_Titles,
	Packet_Play_Server_Command_Suggestions_Response,
	Packet_Play_Server_Commands,
	Packet_Play_Server_Close_Container,
	Packet_Play_Server_Set_Container_Content,
	Packet_Play_Server_Set_Container_Property,
	Packet_Play_Server_Set_Container_Slot,
	Packet_Play_Server_Cookie_Request,
	Packet_Play_Server_Set_Cooldown,
	Packet_Play_Server_Chat_Suggestions,
	Packet_Play_Server_Plugin_Message,
	Packet_Play_Server_Damage_Event,
	Packet_Play_Server_Debug_Sample,
	Packet_Play_Server_Delete_Message,
	Packet_Play_Server_Disconnect,
	Packet_Play_Server_Disguised_Chat_Message,
	Packet_Play_Server_Entity_Event,
	Packet_Play_Server_Synchronise_Entity_Position,
	Packet_Play_Server_Explosion,
	Packet_Play_Server_Unload_Chunk,
	Packet_Play_Server_Game_Event,
	Packet_Play_Server_Open_Horse_Screen,
	Packet_Play_Server_Hurt_Animation,
	Packet_Play_Server_Initialise_World_Border,
	Packet_Play_Server_Keep_Alive,
	Packet_Play_Server_Chunk_Data_And_Update_Light,
	Packet_Play_Server_World_Event,
	Packet_Play_Server_Particle,
	Packet_Play_Server_Update_Light,
	Packet_Play_Server_Login,
	Packet_Play_Server_Map_Data,
	Packet_Play_Server_Merchant_Offers,
	Packet_Play_Server_Update_Entity_Position,
	Packet_Play_Server_Update_Entity_Position_And_Rotation,
	Packet_Play_Server_Update_Entity_Rotation,
	Packet_Play_Server_Move_Vehicle,
	Packet_Play_Server_Open_Book,
	Packet_Play_Server_Open_Screen,
	Packet_Play_Server_Open_Sign_Editor,
	Packet_Play_Server_Ping,
	Packet_Play_Server_Ping_Response,
	Packet_Play_Server_Place_Ghost_Recipe,
	Packet_Play_Server_Player_Abilities,
	Packet_Play_Server_Player_Chat_Message,
	Packet_Play_Server_End_Combat,
	Packet_Play_Server_Enter_Combat,
	Packet_Play_Server_Combat_Death,
	Packet_Play_Server_Player_Info_Remove,
	Packet_Play_Server_Player_Info_Update,
	Packet_Play_Server_Look_At,
	Packet_Play_Server_Synchronise_Player_Position,
	Packet_Play_Server_Update_Recipe_Book,
	Packet_Play_Server_Remove_Entities,
	Packet_Play_Server_Remove_Entity_Effect,
	Packet_Play_Server_Reset_Score,
	Packet_Play_Server_Remove_Resource_Pack,
	Packet_Play_Server_Add_Resource_Pack,
	Packet_Play_Server_Respawn,
	Packet_Play_Server_Set_Head_Rotation,
	Packet_Play_Server_Update_Section_Blocks,
	Packet_Play_Server_Select_Advancements_Tab,
	Packet_Play_Server_Server_Data,
	Packet_Play_Server_Set_Action_Bar_Text,
	Packet_Play_Server_Set_Border_Center,
	Packet_Play_Server_Set_Border_Lerp_Size,
	Packet_Play_Server_Set_Border_Size,
	Packet_Play_Server_Set_Border_Warning_Delay,
	Packet_Play_Server_Set_Border_Warning_Distance,
	Packet_Play_Server_Set_Camera,
	Packet_Play_Server_Set_Held_Item,
	Packet_Play_Server_Set_Center_Chunk,
	Packet_Play_Server_Set_Render_Distance,
	Packet_Play_Server_Set_Default_Spawn_Position,
	Packet_Play_Server_Display_Objective,
	Packet_Play_Server_Set_Entity_Metadata,
	Packet_Play_Server_Link_Entities,
	Packet_Play_Server_Set_Entity_Velocity,
	Packet_Play_Server_Set_Equipment,
	Packet_Play_Server_Set_Experience,
	Packet_Play_Server_Set_Health,
	Packet_Play_Server_Update_Objectives,
	Packet_Play_Server_Set_Passengers,
	Packet_Play_Server_Update_Teams,
	Packet_Play_Server_Update_Score,
	Packet_Play_Server_Set_Simulation_Distance,
	Packet_Play_Server_Set_Subtitle_Text,
	Packet_Play_Server_Update_Time,
	Packet_Play_Server_Set_Title_Text,
	Packet_Play_Server_Set_Title_Animation_Times,
	Packet_Play_Server_Entity_Sound_Effect,
	Packet_Play_Server_Sound_Effect,
	Packet_Play_Server_Start_Configuration,
	Packet_Play_Server_Stop_Sound,
	Packet_Play_Server_Store_Cookie,
	Packet_Play_Server_System_Chat_Message,
	Packet_Play_Server_Set_Tab_List_Header_And_Footer,
	Packet_Play_Server_Tag_Query_Response,
	Packet_Play_Server_Pickup_Item,
	Packet_Play_Server_Teleport_Entity,
	Packet_Play_Server_Set_Ticking_State,
	Packet_Play_Server_Step_Tick,
	Packet_Play_Server_Transfer,
	Packet_Play_Server_Update_Advancements,
	Packet_Play_Server_Update_Attributes,
	Packet_Play_Server_Entity_Effect,
	Packet_Play_Server_Update_Recipes,
	Packet_Play_Server_Update_Tags,
	Packet_Play_Server_Projectile_Power,
	Packet_Play_Server_Custom_Report_Details,
	Packet_Play_Server_Server_Links
};
enum Chat_Mode
{
	Chat_Mode_Full,
	Chat_Mode_Commands_Only,
	Chat_Mode_Hidden
};
enum Skin_Part
{
	Skin_Part_Cape = 1,
	Skin_Part_Jacket,
	Skin_Part_Left_Sleeve = 4,
	Skin_Part_Right_Sleeve = 8,
	Skin_Part_Left_Pants_Leg = 16,
	Skin_Part_Right_Pants_Leg = 32,
	Skin_Part_Hat = 64
};
enum Hand
{
	Hand_Left,
	Hand_Right
};
typedef struct
{
	uint8_t locale[9];
	int8_t render_distance;
	// Displayed skin parts (7), main hand (1).
	uint8_t appearance;
	// Chat mode (3), colored chat (1), text filtering (1), server listing (1), particle status (2).
	uint8_t online_interaction;
} PlayerInformation;

VarInt *bullshitcore_network_varint_encode(int32_t value);
int32_t bullshitcore_network_varint_decode(const VarInt * restrict varint, uint8_t * restrict bytes);
VarLong *bullshitcore_network_varlong_encode(int64_t value);
int64_t bullshitcore_network_varlong_decode(const VarLong * restrict varlong, uint8_t * restrict bytes);
String bullshitcore_network_string_java_utf8_encode(UnicodeString codepoints);

#endif
