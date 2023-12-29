#pragma once

#include "PropertyFlags.h"

/******************** GNames Dumping ********************/
void DumpBlocks(ofstream& Dump, uintptr_t FNamePool, UINT32 blockId, UINT32 blockSize) {
    uintptr_t block = Read<uintptr_t>(FNamePool + Offsets::FNamePoolToBlocks + (blockId * Offsets::PointerSize));

    UINT32 offset = 0;
    UINT32 end = blockSize - (UINT32)Offsets::FNameEntryToString;

    while (offset < end) {
        uintptr_t FNameEntry = block + offset;
        UINT16 FNameEntryHeader = Read<INT16>(FNameEntry + Offsets::FNameEntryHeader);
        uintptr_t strPtr = FNameEntry + Offsets::FNameEntryToString;
        UINT16 strLen = FNameEntryHeader >> Offsets::FNameEntryToLenBit;
        if (strLen) {
            UINT32 index = (blockId << 16 | (offset / (UINT32)Offsets::FNameStride));
            bool wide = FNameEntryHeader & 1;
            if (wide) {
                strLen = strLen * sizeof(wchar_t);
            }

            string name = ReadString(strPtr, strLen);
            if (Offsets::IsDecrypt) {
                name = FName::XorCypher(name);
            }

            Dump << "[" << std::hex << index << std::dec << "]" << (wide ? "wide" : "") << ": " << name << endl;
            
            //Next
            UINT32 bytes = (UINT32)Offsets::FNameEntryToString + strLen;
            UINT32 stride = (UINT32)Offsets::FNameStride;
            offset += (bytes + stride - 1u) & ~(stride - 1u);//align
        }
        else {// Null-terminator entry found
            break;
        }
    }
}

void DumpStrings(string out) {
    ofstream Dump(out + "/NameDump.txt", ofstream::out);
    if (Dump.is_open()) {
        uintptr_t FNamePool = (Offsets::ModuleBase + Offsets::GNames) + Offsets::GNamesToFNamePool;

        UINT32 BlockSize = (UINT32)Offsets::FNameStride * 65536;
        UINT32 CurrentBlock = Read<UINT32>(FNamePool + Offsets::FNamePoolToCurrentBlock);
        UINT32 CurrentByteCursor = Read<UINT32>(FNamePool + Offsets::FNamePoolToCurrentByteCursor);
        Dump << "FName::Index: String" << endl;

        //All Blocks Except Current
        for (UINT32 BlockIdx = 0; BlockIdx < CurrentBlock; ++BlockIdx) {
            DumpBlocks(Dump, FNamePool, BlockIdx, BlockSize);
        }

        //Last Block
        DumpBlocks(Dump, FNamePool, CurrentBlock, CurrentByteCursor);

        Dump.close();
    }
    cout << "Strings Dumped" << endl;
}

void DumpObjects(string out) {
    ofstream Dump(out + "/ObjectsDump.txt", ofstream::out);
    if (Dump.is_open()) {
        INT32 count = GetObjectCount();
        if (count < 10) {
            cout << "Object count is too small. count=" << std::dec << count << endl;
        }
        for (INT32 i = 0; i < count; i++) {
            uintptr_t object = GetUObjectFromID(i);
            if (UObject::isValid(object)) {
                Dump << std::dec << "[" << i << "]:" << endl;
                Dump << "Name: " << UObject::getNameString(object).c_str() << endl;
                Dump << "Class: " << UObject::getClassNameString(object).c_str() << endl;
                Dump << "ObjectPtr: 0x" << std::hex << object << endl;
                Dump << "ClassPtr: 0x" << std::hex << UObject::getClass(object) << endl;
                Dump << endl;
            }
        }
        Dump.close();
    }
    cout << "Objects Dumped" << endl;
}

void DumpSDK(string out) {

    INT32 count = GetObjectCount();
    if (count < 10) {
        cout << "Object count is too small. count=" << std::dec << count << endl;
    }
    SDKInfo sdk;
    for (INT32 i = 0; i < count; i++) {
        uintptr_t object = GetUObjectFromID(i);
        if (UObject::isValid(object)) {
            DumpStructures(sdk, UObject::getClass(object));
        }
    }

    ofstream Dump(out + "/SDK.txt", ofstream::out);
    if (Dump.is_open()) {
        for (uintptr_t clazz : sdk.finalClass)
        {
            Dump << "Class: " << UStruct::getStructClassPath(clazz) << endl;
            for (FName name : sdk.pathMap[clazz]) {
                ClassMember member = sdk.classMap[name];
                Dump << name.GetString() + ":" << endl;
                Dump << member.variable.ToString();
                Dump << member.func.ToString();
            }
            Dump << "--------------------------------\n" << endl;
        }
        Dump.close();
    }
    cout << "SDK Dumped" << endl;
}

void DumpSDKW(string out) {
    SDKInfo sdk;

    //Iterate World
    uintptr_t GWorld = Read<uintptr_t>(Offsets::ModuleBase + Offsets::GWorld);
    DumpStructures(sdk, UObject::getClass(GWorld));

    //Iterate Entities
    TArray actorArray = GetActorArray();
    for (UINT32 i = 0; i < actorArray.count; i++) {
        uintptr_t actor = Read<uintptr_t>(actorArray.addr + (i * Offsets::PointerSize));
        if (UObject::isValid(actor)) {
            DumpStructures(sdk, UObject::getClass(actor));
        }
    }
    
    ofstream Dump(out + "/SDKW.txt", ofstream::out);
    if (Dump.is_open()) {
        for (uintptr_t clazz : sdk.finalClass)
        {
            Dump << "Class: " << UStruct::getStructClassPath(clazz) << endl;
            for (FName name : sdk.pathMap[clazz]) {
                ClassMember member = sdk.classMap[name];
                Dump << name.GetString() + ":" << endl;
                Dump << member.variable.ToString();
                Dump << member.func.ToString();
            }
            Dump << "--------------------------------\n" << endl;
        }
        Dump.close();
    }
    cout << "SDKW Dumped" << endl;
}

void DumpActors(string out) {
    ofstream Dump(out + "/ActorsDump.txt", ofstream::out);
    if (Dump.is_open()) {
        TArray actorArray = GetActorArray();
        StringTable table;
        for (UINT32 i = 0; i < actorArray.count; i++) {
            uintptr_t actor = Read<uintptr_t>(actorArray.addr + (i * Offsets::PointerSize));
            if (UObject::isValid(actor)) {
                table.Add({ "[" + to_string(i) + "]:", UObject::getNameString(actor) , "//" + ToHex(actor) });
            }
        }
        Dump << table.ToString() << endl;
        Dump.close();
    }
    cout << "Actors Dumped" << endl;
}

void DumpEnum(string out) {
    ofstream Dump(out + "/EnumDump.txt", ofstream::out);
    if (Dump.is_open()) {
        INT32 count = GetObjectCount();
        if (count < 10) {
            cout << "Object count is too small. count=" << std::dec << count << endl;
        }
        for (INT32 i = 0; i < count; i++) {
            uintptr_t object = GetUObjectFromID(i);

            if (UObject::isValid(object) && IsEqual(UObject::getClassNameString(object), "Enum"))
            {
                string outerName = "";
                uintptr_t outerClass = UObject::getOuter(object);
                if (UObject::isValid(outerClass)) {
                    outerName = UObject::getNameString(outerClass);
                }
                TArray itemArr = Read<TArray>(object + Offsets::UEnumToNames);

                string enumName = UObject::getNameString(object);
                Dump << outerName << "." << enumName << endl;
                Dump << "enum: " << enumName << endl;

                StringTable table;
                for (UINT32 n = 0; n < itemArr.count; n++)
                {
                    uintptr_t item = itemArr.addr + n * Offsets::UEnumNameSize;
                    string name = Read<FName>(item).GetString();
                    INT64 value = Read<INT64>(item + 8);
                    if (name.find(enumName + "::") == 0) {
                        name = name.substr(enumName.length() + 2);
                    }

                    table.Add({ "    " + name, "= " + to_string(value) + "," });
                }
                Dump << table.ToString() << endl;
            }
        }
        Dump.close();
    }
    cout << "Enum Dumped" << endl;
}



void DumpBoneInfo(unordered_map<string, list<string>>& skeletonMap, string path, uintptr_t character)
{
    uintptr_t Mesh = Read<uintptr_t>(character + Offsets::CharacterToMesh);
    uintptr_t SkeletalMesh = Read<uintptr_t>(Mesh + Offsets::SkinnedMeshToSkeletalMesh);
    TArray RefBoneInfoArray = Read<TArray>(SkeletalMesh + Offsets::SkeletalMeshToFinalRefBoneInfo);

    StringTable table;
    for (UINT32 i = 0; i < RefBoneInfoArray.count; i++)
    {
        uintptr_t boneInfo = RefBoneInfoArray.addr + i * Offsets::BoneInfoSize;
        FName fName = Read<FName>(boneInfo);
        INT32 parentIndex = Read<INT32>(boneInfo + 8);

        table.Add({ "    " + fName.GetString(), "= " + to_string(i) + ",", "//ParentIndex = " + to_string(parentIndex)});
    }
    string bone = table.ToString();
    if (bone.empty()) {
        return;
    }

    if (skeletonMap.find(bone) == skeletonMap.end()) {
        skeletonMap[bone].push_back(path);
    }
    else {
        skeletonMap[bone].remove(path);
        skeletonMap[bone].push_back(path);
    }

}


void DumpSkeleton(string out) {
    //unordered_map<骨骼字符串,list<使用这个骨骼的类>>  //同一个骨骼可能被多个类使用
    unordered_map<string, list<string>> skeletonMap;

    TArray actorArray = GetActorArray();
    for (UINT32 i = 0; i < actorArray.count; i++) {
        uintptr_t actor = Read<uintptr_t>(actorArray.addr + (i * Offsets::PointerSize));
        if (UObject::isValid(actor)) {
            string path = UStruct::getClassPath(actor);
            if (path == "Character.Pawn.Actor.Object" || path.find(".Character.Pawn.Actor.Object") != std::string::npos)
            {
                DumpBoneInfo(skeletonMap, path, actor);
            }
        }
    }

    ofstream Dump(out + "/SkeletonDump.txt", ofstream::out);
    if (Dump.is_open()) {
          
        for (const auto& pair : skeletonMap) {
            for (string path : pair.second) {
                Dump << path << endl;
            }
            Dump << pair.first << endl;
        }
        
        Dump.close();
    }
    cout << "Skeleton Dumped" << endl;
}

