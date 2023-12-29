#include <Windows.h>
#include <sstream>

#include "Dumper.h"


void FindGNames()
{
	for (size_t i = 0; i < 0x12000000; i += Offsets::PointerSize) 
	{
		UINT32 block = 0;
		UINT32 offset = 0;

		uintptr_t FNamePool = ((Offsets::ModuleBase + i) + Offsets::GNamesToFNamePool);

		uintptr_t NamePoolChunk = Read<uintptr_t>(FNamePool + Offsets::FNamePoolToBlocks + (block * Offsets::PointerSize));
		uintptr_t FNameEntry = NamePoolChunk + (Offsets::FNameStride * offset);

		UINT16 FNameEntryHeader = Read<UINT16>(FNameEntry + Offsets::FNameEntryHeader);
		uintptr_t strPtr = FNameEntry + Offsets::FNameEntryToString;
		UINT16 strLen = FNameEntryHeader >> Offsets::FNameEntryToLenBit;

		if (strLen == 4) {
			string name(strLen, '\0');
			bool result = Memory.Read(strPtr, (PBYTE)name.data(), strLen * sizeof(char));
			name.shrink_to_fit();
			if (result && IsEqual(name, "None"))
			{
				Offsets::GNames = i;
				cout << "GName = 0x" << std::hex << i << endl;
				return;
			}
		}
	}
	cout << "GNames not found" << endl;
}

void FindGWorld()
{
	if (Offsets::GNames == NULL) {
		cout << "Can't find GWorld, because GNames is null." << endl;
		return;
	}

	for (size_t i = 0; i < 0x12000000; i += Offsets::PointerSize)
	{
		uintptr_t GWorld = Read<uintptr_t>(Offsets::ModuleBase + i);
		uintptr_t PersistentLevel = Read<uintptr_t>(GWorld + Offsets::UWorldToPersistentLevel);
		TArray actorArray = Read<TArray>(PersistentLevel + Offsets::ULevelToAActors);

		if (actorArray.IsValid()) {
			uintptr_t firstActor = Read<uintptr_t>(actorArray.addr);
			string name = UObject::getNameString(firstActor);
			if (strstr(name.c_str(), "WorldSettings"))
			{
				Offsets::GWorld = i;
				cout << "GWorld = 0x" << std::hex << i << endl;
				return;
			}
		}

	}
	cout << "GWorld not found" << endl;
		
}

void FindTUObjectArray()
{
	if (Offsets::GNames == NULL) {
		cout << "Can't find TUObjectArray, because GNames is null." << endl;
		return;
	}
	for (size_t i = 0; i < 0x12000000; i += Offsets::PointerSize)
	{
		const INT32 index = 0;
		uintptr_t FUObjectArray = (Offsets::ModuleBase + i);
		uintptr_t TUObjectArray = Read<uintptr_t>(FUObjectArray + Offsets::FUObjectArrayToTUObjectArray);
		uintptr_t Chunk = Read<uintptr_t>(TUObjectArray + ((index / 0x10000) * Offsets::PointerSize));
		uintptr_t firstObj = Read<uintptr_t>(Chunk + Offsets::FUObjectItemPadd + ((index % 0x10000) * Offsets::FUObjectItemSize));

		string name = UObject::getNameString(firstObj);
		if (strstr(name.c_str(), "CoreUObject")) {
			string className = UObject::getClassNameString(firstObj);
			if (strstr(className.c_str(), "Package")) {
				Offsets::TUObjectArray = i;
				cout << "TUObjectArray = 0x" << std::hex << i << endl;
				return;
			}
		}
	}
	cout << "TUObjectArray not found" << endl;
}


int main() {

	//初始化
	if (Memory.Init() == false) {
		return 0;
	}
	
	//注意：CreateDirectoryA一次只能创建一个文件夹
	string OutPutDirectory("D:\\Dumped");
	CreateDirectoryA(OutPutDirectory.c_str(), NULL);

	OutPutDirectory = OutPutDirectory + "\\" + Offsets::TargetProcess;
	CreateDirectoryA(OutPutDirectory.c_str(), NULL);

	cout << "\nOutPutDirectory = " << OutPutDirectory << endl;

	cout << "\n USING F KEYS.\n" << endl;

	cout << " F1. Find GNames & GWorld & TUObjectArray" << endl;
	cout << " F2. Dump String" << endl;
	cout << " F3. Dump Objects" << endl;
	cout << " F4. Dump SDK  (Using GUObjectArray)" << endl;
	cout << " F5. Dump SDKW (Using GWorld)" << endl;
	cout << " F6. Dump Actors" << endl;
	cout << " F7. Dump Enum" << endl;
	cout << " F8. Dump Skeleton" << endl;


	while (true) {
		if (GetAsyncKeyState(VK_F1) & 1) {
			Offsets::PrintWarning = false;
			cout << "\nFinding GNames" << endl;
			FindGNames();
			cout << "\nFinding GWorld" << endl;
			FindGWorld();
			cout << "\nFinding TUObjectArray" << endl;
			FindTUObjectArray();
			Offsets::PrintWarning = true;
		}
		if (GetAsyncKeyState(VK_F2) & 1) {
			cout << "\nDumping Strings" << endl;
			DumpStrings(OutPutDirectory);
		}
		if (GetAsyncKeyState(VK_F3) & 1) {
			cout << "\nDumping Objects" << endl;
			DumpObjects(OutPutDirectory);
		}
		if (GetAsyncKeyState(VK_F4) & 1) {
			cout << "\nDumping SDK (using GUObject)" << endl;
			DumpSDK(OutPutDirectory);
		}
		if (GetAsyncKeyState(VK_F5) & 1) {
			cout << "\nDumping SDKW (using GWorld)" << endl;
			DumpSDKW(OutPutDirectory);
		}
		if (GetAsyncKeyState(VK_F6) & 1) {
			cout << "\nDumping Actors" << endl;
			DumpActors(OutPutDirectory);
		}
		if (GetAsyncKeyState(VK_F7) & 1) {
			cout << "\nDumping Enum" << endl;
			DumpEnum(OutPutDirectory);
		}
		if (GetAsyncKeyState(VK_F8) & 1) {
			cout << "\nDumping Skeleton" << endl;
			DumpSkeleton(OutPutDirectory);
		}
		/*
		if (GetAsyncKeyState(VK_END) & 1) {
			exit(0);
		}
		*/
	}
}