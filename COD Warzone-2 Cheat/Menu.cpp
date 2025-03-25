#include "stdafx.h"
#include <fstream>
#include "Menu.h"
#include "imgui/imgui.h"
# include "globals.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "obfuscator.hpp"
#include "xor.hpp"

uint64_t base = (uint64_t)GetModuleHandleA(NULL);

int Players;
bool allowLoadout;
bool TPBots = false;
bool EnableDump = false;
bool EnableDump2 = false;
bool Enable1000 = false;
bool EnableBP100 = false;
bool EnableeBP100 = false;
bool EnableWeaponXP = false;
bool DMUUnlock = false;
bool WZBypass = false;


struct StringTableEntry
{
	char* value;
	int hash;
};

struct StringTable
{
	char* name;
	int columnCount;
	int rowCount;
	StringTableEntry* values;
};

enum structuredDataType_t
{
	STRUCTURED_DATA_INT = 0,
	STRUCTURED_DATA_BYTE = 1,
	STRUCTURED_DATA_BOOL = 2,
	STRUCTURED_DATA_STRING = 3,
	STRUCTURED_DATA_ENUM = 4,
	STRUCTURED_DATA_STRUCT = 5,
	STRUCTURED_DATA_INDEXEDARR = 6,
	STRUCTURED_DATA_ENUMARR = 7,
	STRUCTURED_DATA_FLOAT = 8,
	STRUCTURED_DATA_SHORT = 9
};

struct structuredDataItem_t
{
	structuredDataType_t type;
	union
	{
		int index;
	};
	int offset;
};

struct enumEntry
{
	char* name;
	short index;
	short _pad;
};

struct definedEnum
{
	int entryCount;
	long unknown4;
	enumEntry* entries;        //Array - Count = unknownCount5;
};

struct structureEntry
{
	char* name;
	structuredDataItem_t item;
};

struct definedStructure
{
	int entryCount;
	structureEntry* entries;
	int size;
	long unknown7;
};

struct structuredDataIndexedArray_t
{
	int numItems;
	structuredDataItem_t item;
};

struct structuredDataEnumArray_t
{
	int enumIndex;
	structuredDataItem_t item;
};

struct StructuredData
{
	int version;
	unsigned int hash;
	int enumCount;
	definedEnum* enums;    //Array - Count = unknownCount1;
	int structureCount;
	definedStructure* structures;    //Array - Count = unknownCount2;
	int indexedArrayCount;
	structuredDataIndexedArray_t* indexedArrays;                    //Array of data - Total size = unknownCount3 << 4
	int enumArrayCount;
	structuredDataEnumArray_t* enumArrays;                    //Array of data - Total size = unknownCount4 << 4
	structuredDataItem_t rootItem;
};

struct StructuredDataDef
{
	char* name;
	int structureCount;
	StructuredData* internalStructure; //Array-Count = unknownCount1;
};

union XAssetHeader
{
	StringTable* stringTable;
	StructuredDataDef* structuredData;
};

void INTERGRITY_Style() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = ImColor(1, 1, 1, 160);
	style.Colors[ImGuiCol_Border] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_Button] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_TitleBg] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_TitleBgActive] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_TabHovered] = ImColor(68, 0, 139, 185);
	style.Colors[ImGuiCol_Tab] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_TabActive] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_Separator] = ImColor(68, 0, 139, 255);
	style.Colors[ImGuiCol_SeparatorActive] = ImColor(68, 0, 139, 255);
}

void REVERSE_Style() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = ImColor(1, 1, 1, 160);
	style.Colors[ImGuiCol_Border] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_Button] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_TitleBg] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_TitleBgActive] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_TabHovered] = ImColor(255, 0, 0, 185);
	style.Colors[ImGuiCol_Tab] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_TabActive] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_Separator] = ImColor(255, 0, 0, 255);
	style.Colors[ImGuiCol_SeparatorActive] = ImColor(255, 0, 0, 255);
}

union VariableUnion {
	int intValue;
	unsigned int uintValue;
	float floatValue;
	unsigned int stringValue;
	const float* vectorValue;
	const char* codePosValue;
	unsigned __int64 scriptCodePosValue;
	unsigned int pointerValue;
	unsigned int entityOffset;
};

struct VariableValue {
	VariableUnion u;
	int type;
};

struct scrContext_t {
	char __padding0000[0xB9C0];
	VariableValue* top;
};

struct scr_entref_t {
	unsigned int entNum;
	unsigned int entClass;
};

struct DDLState {
	int isValid;
	int offset;
	int arrayIndex;
	int member;
};


struct DDLContext {
	char buff;
	int length;
	int def;
	int accessCB;
	int userdata;
	int obsfucation;
	int randomint;
};

uintptr_t g_entities = *(uintptr_t*)(base + 0x1A99EF88); // 48 8B 15 ? ? ? ? 41 B9 ? ? ? ? C7 07 ? ? ? ?

__int64 GetSvClient(int clientNum) {
	uintptr_t player = *reinterpret_cast<uintptr_t*>(base + (clientNum * 8) + 0x1DDE6AF0); // 4C 8D 35 ? ? ? ? 66 90 49 8B 0C DE
	if (player) {
		return player;
	}
	else {
		return 0;
	}
}

void Scr_AddInt(scrContext_t* scrContext, int value) {
	uintptr_t address = base + 0x3687C50;
	((void(*)(scrContext_t*, int))address)(scrContext, value);
}

__int64 Scr_AddString(scrContext_t* scrContext, const char* param) { // 5E E9 ? ? ? ? 40 53 48 83 EC 20 33 D2
	uintptr_t address = base + 0x3687A70;
	return ((__int64 (*)(scrContext_t*, const char*))address)(scrContext, param);
}

const char* SL_ConvertToString(uint32_t stringValue) { // 48 8B F8 E8 ? ? ? ? 4C 8B C8 48 2B F8 
	uintptr_t address = base + 0x367F8A0;
	return ((const char* (*)(uint32_t))address)(stringValue);
}

unsigned int SL_GetString(const char* str, unsigned int user) { // E8 ? ? ? ? 48 8B 55 A0 4C 8D 4D A8
	uintptr_t address = base + 0x367FCB0;
	return ((unsigned int(*)(const char*, unsigned int))address)(str, user);
}

const char* Dvar_GetVariantStringWithDefault(const char* dvarName, const char* defaultValue) { // 40 53 48 83 EC 40 48 8B DA E8 ? ? ? ?
	uintptr_t address = base + 0x39D6160;
	return ((const char* (*)(const char*, const char*))address)(dvarName, defaultValue);
}

const char* Scr_GetString(scrContext_t* scrContext, unsigned int index) {
	uintptr_t address = base + 0x3689840;
	return ((const char* (*)(scrContext_t*, unsigned int))address)(scrContext, index);
}

__int64 Scr_GetNumParams(scrContext_t* scrContext) { // E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 48 8B 03 48 8B CB
	uintptr_t address = base + 0x3689610;
	return ((__int64 (*)(scrContext_t*))address)(scrContext);
}

void ReportUserChallenge(scrContext_t* scrContext, scr_entref_t entref) { // 
	uintptr_t address = base + 0x3334640;
	((void(*)(scrContext_t*, scr_entref_t))address)(scrContext, entref);
}

__int64 Scr_ClearOutParams(scrContext_t* scrContext) { // E8 ? ? ? ? 33 ED 85 F6
	uintptr_t address = base + 0x3688300;
	return ((__int64 (*)(scrContext_t*))address)(scrContext);
}

__int64 Scr_GetConstString(scrContext_t* scrContext, unsigned int index) { // E8 ? ? ? ? 0F BF 1B 8B F8 E8 ? ? ? ? 44 8B C3 
	uintptr_t address = base + 0x3689140;
	return ((__int64 (*)(scrContext_t*, unsigned int))address)(scrContext, index);
}

char* StringTable_GetColumnValueForRow(__int64 table, int row, int colum) { // 4C 8B E1 E8 ? ? ? ? 41 B8 ? ? ? ?
	uintptr_t address = base + 0x39CB5D0;
	return ((char* (*)(__int64, int, int))address)(table, row, colum);
}

__int64 Dvar_SetString(const char* dvarname, const char* value) { // E8 ? ? ? ? 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B C8 48 8B 5C 24 ?
	uintptr_t address = base + 0x39D5090; // 
	return ((__int64 (*)(const char*, const char*))address)(dvarname, value);
}

__int64 SL_GetRawHash(int hash) { // E8 ? ? ? ? 4C 8D 8C 24 ? ? ? ? 89 84 24 ? ? ? ?
	uintptr_t address = base + 0x3680440;
	return ((__int64 (*)(int))address)(hash);
}

bool CL_PlayerData_GetDDLBuffer(DDLContext* context, int controllerindex, int stats_source, unsigned int statsGroup) { // 
	uintptr_t address = base + 0x41BF320;
	return ((bool (*)(DDLContext*, int, int, unsigned int))address)(context, controllerindex, stats_source, statsGroup);
}

__int64 LiveStorage_GetActiveStatsSource(int controllerindex) { // 89 44 24 78 E8 ? ? ? ? B9 ? ? ? ?
	uintptr_t address = base + 0x3680440;
	return ((__int64 (*)(int))address)(controllerindex);
}

void StringTable_GetAsset(const char* filename, StringTable **tablePtr) { // E8 ? ? ? ? E8 ? ? ? ? 48 8B 4C 24 ? 41 B9 ? ? ? ?
	uintptr_t address = base + 0x39CB590;
	((void (*)(const char*, StringTable**))address)(filename, tablePtr);
}

void LiveStorage_InitializeDDLStateForStatsGroup(__int64 ddldef, DDLState* state, unsigned int statsGroup) { // 44 8B C3 E8 ? ? ? ? 8B CD 
	uintptr_t address = base + 0x2EF18B0;
	((void (*)(__int64, DDLState*, unsigned int))address)(ddldef, state, statsGroup);
}

__int64 Com_PlayerData_GetDefForStatsGroup(int statsGroup) { // 8B D8 E8 ? ? ? ? 44 8B C3 48 8D 54 24 ?
	uintptr_t address = base + 0x2EEF3F0;
	return ((__int64 (*)(int))address)(statsGroup);
}

bool DDL_MoveToNameByHash(const DDLState* fromState, DDLState* toState, unsigned int nameHash, const char* name) { //
	uintptr_t address = base + 0x6A27B20;
	return ((bool (*)(const DDLState*, DDLState*, unsigned int, const char*))address)(fromState, toState, nameHash, name);
}

bool DDL_MoveToIndex(const DDLState* fromState, DDLState* toState, int index) { //
	uintptr_t address = base + 0x6A27AF0;
	return ((bool (*)(const DDLState*, DDLState*, int))address)(fromState, toState, index);
}

bool b_menu_open = true;
bool b_debug_open = false;


void MaxWeapons(scrContext_t* scrContext, scr_entref_t entref) {
	
	const char* ChallengeList[3];

	ChallengeList[0] = "iw8_me_t9sledgehammer_mp";
	ChallengeList[1] = "iw8_me_t9wakizashi_mp";
	ChallengeList[2] = "iw8_me_t9machete_mp";
	ChallengeList[3] = "iw8_me_t9etool_mp";

	VariableValue* challenge_name = &scrContext->top[-3];
	VariableValue* challenge_value = &scrContext->top[-4];

	const char* Challenge_name = SL_ConvertToString(challenge_name->u.uintValue);
	const char* Challenge_value = SL_ConvertToString(challenge_value->u.uintValue);

	for (int i = 0; i < 3; i++)
	{
		challenge_value->u.uintValue = SL_GetString("50000000", 0);
		challenge_name->u.uintValue = SL_GetString(ChallengeList[i], 0);
	}
}

/*
// Hooking
void Gscr_MainMP_SetPlayerData(scrContext_t* scrContext, scr_entref_t entref) {

	DDLState state;
	DDLContext ddlContext;
	unsigned int StatsGroup;
	unsigned int GetString;
	unsigned int RawHash;
	__int64 Buf;
	__int64 Len;
	__int64 DdlDef;
	__int64 TablePtr;

	if (EnableDump2) {

		GetString = Scr_GetConstString(scrContext, 0);
		RawHash = SL_GetRawHash(GetString);
		StatsGroup = Com_PlayerData_FindStatsGroupByHash(RawHash);

		DdlDef = Com_PlayerData_GetDefForStatsGroup(StatsGroup);
		Buf = SV_ClientMP_GetClientPlayerDataBuffer(0, StatsGroup);
		Len = LiveStorage_GetPlayerDataBufferSize(StatsGroup);

		Com_DDL_CreateContext(Buf, Len, DdlDef, &ddlContext, 0, 0);

		std::ofstream outputFile4;
		outputFile4.open("C:\\Users\\Intergity\\Desktop\\SetPlayerData.txt", std::ios::binary |
			std::ios::app);
		outputFile4 << Scr_GetString(scrContext, 0) << "ARG 1:" << Scr_GetString(scrContext, 0) << "ARG: 2" << Scr_GetString(scrContext, 0) << "ARG: 3" << std::endl;
		outputFile4.close();

		state.isValid = 0;
		state.offset = 0;
		state.arrayIndex = -1;

		LiveStorage_InitializeDDLStateForStatsGroup(DdlDef, &state, StatsGroup);

		GScr_Main_SetDDL(scrContext, &state, DdlDef, &ddlContext, "SetPlayerData", 1);
	}
}

void Gscr_MainMP_GetPlayerData(scrContext_t* scrContext, scr_entref_t entref) {

	DDLState state;
	DDLContext ddlContext;
	unsigned int StatsGroup;
	unsigned int GetString;
	unsigned int RawHash;
	__int64 Buf;
	__int64 Len;
	__int64 DdlDef;

	if (EnableDump) {
		GetString = Scr_GetConstString(scrContext, 0);
		RawHash = SL_GetRawHash(GetString);
		StatsGroup = Com_PlayerData_FindStatsGroupByHash(RawHash);

		DdlDef = Com_PlayerData_GetDefForStatsGroup(StatsGroup);
		Buf = SV_ClientMP_GetClientPlayerDataBuffer(0, StatsGroup);
		Len = LiveStorage_GetPlayerDataBufferSize(StatsGroup);
		
		Com_DDL_CreateContext(Buf, Len, DdlDef, &ddlContext, 0, 0);
				
		std::ofstream outputFile3;
		outputFile3.open("C:\\Users\\Intergity\\Desktop\\GetPlayerData.txt", std::ios::binary |
		std::ios::app);
		outputFile3 << "ARG 1:" << Scr_GetString(scrContext, 0) << "ARG 2:" << Scr_GetString(scrContext, 1) << "ARG: 3" << Scr_GetString(scrContext, 2) << "ARG: 4" << Scr_GetString(scrContext, 3) << "ARG: 5" << Scr_GetString(scrContext, 4) << std::endl;
		outputFile3.close();

		state.isValid = 0;
		state.offset = 0;
		state.arrayIndex = -1;

		LiveStorage_InitializeDDLStateForStatsGroup(DdlDef, &state, StatsGroup);
		GScr_Main_GetDDL(scrContext, &state, DdlDef, &ddlContext, "GetPlayerData", 1);
	}
}
*/
void ChallengeEvent(scrContext_t* scrContext, scr_entref_t entref) {

	uintptr_t scrContextAdr = (uintptr_t)(scrContext);
	int arg_count = *reinterpret_cast<int*>(scrContextAdr + 0x0B9CC);
	
	char dest[32];
	StringTable* tablePtr;

	VariableValue* challenge_id = &scrContext->top[0];
	VariableValue* playerxp = &scrContext->top[-1];
	VariableValue* weaponid = &scrContext->top[-3];
	VariableValue* weaponxp = &scrContext->top[-4];
	VariableValue* bpxp = &scrContext->top[-5];

	const char* ChallengeID = SL_ConvertToString(challenge_id->u.uintValue);
	const char* PlayerXP = SL_ConvertToString(playerxp->u.uintValue);
	const char* WeaponID = SL_ConvertToString(weaponid->u.uintValue);
	const char* WeaponXP = SL_ConvertToString(weaponxp->u.uintValue);
	const char* BattlepassXP = SL_ConvertToString(bpxp->u.uintValue);


	if (arg_count == 14) {

		if (Enable1000) {
			playerxp->u.uintValue = SL_GetString("50000000", 0);
			ReportUserChallenge(scrContext, entref);

		}

		if (EnableBP100) {
			challenge_id->u.uintValue = SL_GetString("battlepass", 0);
			playerxp->u.uintValue = SL_GetString("50000000", 0);
			bpxp->u.uintValue = SL_GetString("50000000", 0);
			ReportUserChallenge(scrContext, entref);

		}

		if (EnableeBP100) {
			challenge_id->u.uintValue = SL_GetString("mp_addxp", 0);
			playerxp->u.uintValue = SL_GetString("50000000", 0);
			bpxp->u.uintValue = SL_GetString("50000000", 0);
			ReportUserChallenge(scrContext, entref);
		}

		if (EnableWeaponXP) {
			weaponxp->u.uintValue = SL_GetString("50000000", 0);

			StringTable_GetAsset("mp/statstable.csv", &tablePtr);

		}

		if (DMUUnlock) {
			DMUUnlock = false;
			MaxWeapons(scrContext, entref);
		}

		std::ofstream outputFile2;
		outputFile2.open("C:\\Users\\Intergity\\Desktop\\DumpUnlocks.txt", std::ios::binary |
		std::ios::app);
		outputFile2  << "ARG 1:" << Scr_GetString(scrContext, 0) << "ARG2:" << Scr_GetString(scrContext, 1) << "ARG3:" << Scr_GetString(scrContext, 2) << "ARG4:" << Scr_GetString(scrContext, 3) << "ARG5:" << Scr_GetString(scrContext, 4) << "ARG6:" << Scr_GetString(scrContext, 5) << "ARG7:" << Scr_GetString(scrContext, 6) << std::endl;
		outputFile2.close();

	}

	ReportUserChallenge(scrContext, entref);
}

void GScr_GetDvarInt_Hooked(scrContext_t* scrContext) {
	const char* default_value = "";
	uintptr_t scrContextAddr = (uintptr_t)(scrContext);
	int arg_count = *reinterpret_cast<int*>(scrContextAddr + 0x0B9CC);
	VariableValue* top = &scrContext->top[-0];
	const char* dvar_string = SL_ConvertToString(top->u.intValue);

	if (Players > 18) {

		Players = 0;
	}

	if (TPBots) {
		GetSvClient(Players) + 0x30;
		GetSvClient(Players) + 0x34;
		GetSvClient(Players) + 0x38;
	}
	/*
	if (WZBypass) {

		if (strcmp(dvar_string, "NSQLTTMRMP")) {
			Scr_AddString(scrContext, "mp_shipment");
			return;
		}

		if (strcmp(dvar_string, "MOLPOSLOMO")) {
			Scr_AddString(scrContext, "dm");
			return;
		}
	}
	*/
	if (!allowLoadout)
	{
		if (strcmp(dvar_string, "LSTLQTSSRM") == 0) {
			Scr_AddInt(scrContext, 1);
			allowLoadout = true;
			return;
		}
	}
	else {
		if (strcmp(dvar_string, "LSTLQTSSRM") == 0) {
			Scr_AddInt(scrContext, 0);
			return;
		}
	}

	if (strcmp(dvar_string, "MONKPPPQR") == 0) {
		allowLoadout = false;
		return;
	}

	if (strcmp(dvar_string, "LKKNORQKTP") == 0) { // Player XP Scale
		Scr_AddInt(scrContext, 2);
		return;
	}

	if (strcmp(dvar_string, "PMORNPNTK") == 0) { // Weapon XP Scale
		Scr_AddInt(scrContext, 2);
		return;
	}

	if (strcmp(dvar_string, "LTKKKPSRSK") == 0) { // BP XP Scale
		Scr_AddInt(scrContext, 2);
		return;
	}

	if (strcmp(dvar_string, "scr_disable_anti_afk") == 0) {
		Scr_AddInt(scrContext, 1);
		return;
	}

	if (arg_count != 1) {
		if (arg_count != 2) {
			return;
		}
		default_value = Scr_GetString(scrContext, 1);
	}
	const char* dvar_name = Scr_GetString(scrContext, 0);
	const char* dvar_value = Dvar_GetVariantStringWithDefault(dvar_name, default_value);
	Scr_AddInt(scrContext, atoi(dvar_value));
}

namespace g_menu
{
	void menu()
	{
		if (GetAsyncKeyState(VK_INSERT) & 0x1)
		{
			b_menu_open = !b_menu_open;
		}
		if (b_menu_open)
		{
			ImGui::Begin(xorstr("REVERSE'S : Instant XP Tool Edition"), &b_menu_open, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
			ImGui::SetWindowSize(ImVec2(270, 290));
			REVERSE_Style();
		
			ImGui::Spacing();
			if (ImGui::Button(xorstr("Enable Everything"))) {
				*reinterpret_cast<uintptr_t*>(base + 0x9B1ECD0) = (uintptr_t)(GScr_GetDvarInt_Hooked);
			//	*reinterpret_cast<uintptr_t*>(base + 0x9ED3068) = (uintptr_t)(Gscr_MainMP_GetPlayerData);
			//	*reinterpret_cast<uintptr_t*>(base + 0x9ED3080) = (uintptr_t)(Gscr_MainMP_SetPlayerData);
				*reinterpret_cast<uintptr_t*>(base + 0x9B26B70) = (uintptr_t)(ChallengeEvent); // 00 00 00 00 00 00 00 00 00 00 1A 86 00 00 00 00 00 00

			}
			ImGui::SameLine();
			if (ImGui::Button(xorstr("Teleport Bots to Host"))) {
				TPBots = true;
			}
		//	ImGui::Spacing();
		//	if (ImGui::Button(xorstr("Enable PlayerData Dumps"))) {
		//		EnableDump = true;
//				EnableDump2 = true;
		//	}
			ImGui::Spacing();
		//	if (ImGui::Button(xorstr("Warzone Bypass"))) {
		//		WZBypass = true;
		//	}
			ImGui::Spacing();

			ImGui::NewLine();

			if (ImGui::Button(xorstr("Instant 1000"))) {
				Enable1000 = true;
				
			}
			ImGui::SameLine();
			if (ImGui::Button(xorstr("Instant BP Tier 100 [ MIGHT NOT WORK ]"))) {
				EnableBP100 = true;

			}
			ImGui::NewLine();
			if (ImGui::Button(xorstr("Instant BP Tier 100 [ TEST ]"))) {
				EnableeBP100 = true;

			}
			/*
			ImGui::NewLine();
			ImGui::Spacing();
			if (ImGui::Button(xorstr("Max CW Weapons"))) {
				EnableWeaponXP = true;
			}
			ImGui::SameLine();
			if (ImGui::Button(xorstr("Unlock DM Ultra"))) {
				DMUUnlock = true;
			}
			if (ImGui::Button(xorstr("Complete all Daily/Weekly Challenges"))) {

				DDLContext context;
				__int64 ddlDef;
				DDLState state;
				DDLState fromState;
				DDLState toState;
				unsigned int challenge_state;
				unsigned int challenge_progress;
				unsigned int dailychallenge_id;
				unsigned int StatsSource;
				StringTable* tablePtr;

				StatsSource = LiveStorage_GetActiveStatsSource(0);
				if (CL_PlayerData_GetDDLBuffer(&context, 0, StatsSource, 0)) {

					ddlDef = Com_PlayerData_GetDefForStatsGroup(0);

					StringTable_GetAsset("mp/dailyChallengesTable.csv", &tablePtr);
					LiveStorage_InitializeDDLStateForStatsGroup(ddlDef, &state, 0);

					challenge_state = SL_GetRawHash((int)"challengeState");
					DDL_MoveToNameByHash(&state, &toState, challenge_state, 0);

					challenge_progress = SL_GetRawHash((int)"challengeProgress");
					DDL_MoveToNameByHash(&state, &toState, challenge_progress, 0);

					dailychallenge_id = SL_GetRawHash((int)"dailyChallengeId");
					DDL_MoveToNameByHash(&state, &fromState, dailychallenge_id, 0);

					for (int i = 0; i < 3; ++i)
					{
						DDL_MoveToIndex(&fromState, &toState, i);
					}
				}

			}
			ImGui::Spacing();
			if (ImGui::Button(xorstr("Set 1337 Stats Editing"))) {

				unsigned int KDRatio;
				unsigned int CurrentWinStreak;
				unsigned int GamesPlayed;
				unsigned int WinLossRatio;
				DDLContext context;

				KDRatio = SL_GetRawHash((int)"kdRatio");
				CurrentWinStreak = SL_GetRawHash((int)"currentWinStreak");
				GamesPlayed = SL_GetRawHash((int)"gamesPlayed");
				WinLossRatio = SL_GetRawHash((int)"winLossRatio");


			}
			*/
			ImGui::End();
		}
	}
}