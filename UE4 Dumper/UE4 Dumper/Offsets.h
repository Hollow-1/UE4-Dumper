#pragma once

//只支持Dump UE 4.23 之后的游戏
//Only versions after UE 4.23 are supported.

namespace Offsets {

    bool IsDecrypt = false;
    bool PrintWarning = true;
    
    DWORD ProcessID;
    uintptr_t ModuleBase;

    const char* TargetProcess = "ReadyOrNot-Win64-Shipping.exe";

    uintptr_t GWorld = 0x67efad0;
    uintptr_t GNames = 0x666ED80;
    uintptr_t TUObjectArray = 0x66ab0e0;
    

    /*
    const char* TargetProcess = "DreadHunger-Win64-Shipping.exe";

    uintptr_t GWorld = 0x4CE9750;
    uintptr_t GNames = 0x4B99F80;
    uintptr_t TUObjectArray = 0x4BB2788;
    */

    uintptr_t PointerSize = 0x8;//0x8 = 64 Bit and 0x4 = 32 Bit

    //Class: FNamePool
    uintptr_t FNameStride = 0x2;
    uintptr_t GNamesToFNamePool = 0x0;
    uintptr_t FNamePoolToCurrentBlock = 0x8;
    uintptr_t FNamePoolToCurrentByteCursor = 0xC;
    uintptr_t FNamePoolToBlocks = 0x10;

    //Class: FNameEntry
    uintptr_t FNameEntryHeader = 0x0;//Not needed for most games
    uintptr_t FNameEntryToLenBit = 0x6;
    uintptr_t FNameEntryToString = 0x2;

    //Class: FUObjectArray
    uintptr_t FUObjectArrayToTUObjectArray = 0x0;//Sometimes 0x10
    uintptr_t FUObjectItemPadd = 0x0;
    uintptr_t FUObjectItemSize = 0x18;

    //Class: TUObjectArray
    uintptr_t TUObjectArrayToNumElements = 0x14;

    //Class: UObject
    uintptr_t UObjectToInternalIndex = 0xC;
    uintptr_t UObjectToClassPrivate = 0x10;
    uintptr_t UObjectToFNamePrivate = 0x18;
    uintptr_t UObjectToOuterPrivate = 0x20;

    //Class: FField
    uintptr_t FFieldToClass = 0x8;
    uintptr_t FFieldToNext = 0x20;
    uintptr_t FFieldToName = FFieldToNext + PointerSize;

    //Class: UField
    uintptr_t UFieldToNext = 0x28;

    //Class: UStruct
    uintptr_t UStructToSuperStruct = 0x40;
    uintptr_t UStructToChildren = UStructToSuperStruct + PointerSize;
    uintptr_t UStructToChildProperties = 0x50;

    //Class: UFunction
    uintptr_t UFunctionToFunctionFlags = 0xB0;
    uintptr_t UFunctionToFunc = UFunctionToFunctionFlags + UFieldToNext;

    //Class: UProperty
    uintptr_t UPropertyToElementSize = 0x3C;
    uintptr_t UPropertyToPropertyFlags = 0x40;
    uintptr_t UPropertyToOffsetInternal = 0x4C;

    //Class: UBoolProperty
    uintptr_t UBoolPropertyToFieldSize = 0x78;
    uintptr_t UBoolPropertyToByteOffset = UBoolPropertyToFieldSize + 0x1;
    uintptr_t UBoolPropertyToByteMask = UBoolPropertyToByteOffset + 0x1;
    uintptr_t UBoolPropertyToFieldMask = UBoolPropertyToByteMask + 0x1;

    //Class: UObjectProperty
    uintptr_t UObjectPropertyToPropertyClass = 0x78;

    //Class: UClassProperty
    uintptr_t UClassPropertyToMetaClass = 0x80;

    //Class: UInterfaceProperty
    uintptr_t UInterfacePropertyToInterfaceClass = 0x78;

    //Class: UArrayProperty
    uintptr_t UArrayPropertyToInnerProperty = 0x78;

    //Class: UMapProperty
    uintptr_t UMapPropertyToKeyProp = 0x78;
    uintptr_t UMapPropertyToValueProp = UMapPropertyToKeyProp + PointerSize;

    //Class: USetProperty
    uintptr_t USetPropertyToElementProp = 0x78;

    //Class: UStructProperty
    uintptr_t UStructPropertyToStruct = 0x78;

    //Class: UWorld
    uintptr_t UWorldToPersistentLevel = 0x30;

    //Class: ULevel
    uintptr_t ULevelToAActors = 0x98;

    //Enum
    uintptr_t UEnumToNames = 0x40;
    uintptr_t UEnumNameSize = 0x10;

    //Skeleton
    uintptr_t CharacterToMesh = 0x280;
    uintptr_t SkinnedMeshToSkeletalMesh = 0x480;
    uintptr_t SkeletalMeshToFinalRefBoneInfo = 0x1d0; //UE4.26 => 0x1c8;  UE4.27 => 0x1d0
    uintptr_t BoneInfoSize = 0xc;

}
