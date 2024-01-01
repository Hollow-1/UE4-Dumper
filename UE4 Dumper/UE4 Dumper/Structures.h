#pragma once

#include "Memory.h"
#include "StringTable.h"
#include <unordered_map>
#include <map>
#include <string>

class TArray
{
public:
    uintptr_t addr;
    UINT32 count;
    UINT32 max;
    bool IsValid() {
        return addr != NULL && count <= max;
    }
};


class FName
{
public:

    UINT32 index;
    UINT32 number;

    FName() : index(0), number(0) {}
    FName(UINT32 index, UINT32 number) : index(index), number(number) {}

    //为了添加进 std::map， 需要重载 < 运算符，map只使用 < 就可以进行各种比较
    bool operator<(const FName& other)const {
        if (index != other.index) {
            return index < other.index;
        }
        return number < other.number;
    }

    string GetString() {
        string name = GetString(index);
        if (number != 0) {
            name = name + "_" + std::to_string(number - 1);
        }
        return name;
    }

    static string GetString(UINT32 index) {

        //Cache index and name
        static unordered_map<UINT32, string> nameCacheMap;

        if (nameCacheMap.find(index) != nameCacheMap.end()) {
            return nameCacheMap[index];
        }
        UINT32 block = index >> 16;
        UINT32 offset = index & 65535;

        offset = offset * (UINT32)Offsets::FNameStride;
        uintptr_t FNamePool = ((Offsets::ModuleBase + Offsets::GNames) + Offsets::GNamesToFNamePool);
        UINT32 CurrentBlock = Read<UINT32>(FNamePool + Offsets::FNamePoolToCurrentBlock);
        UINT32 CurrentByteCursor = Read<UINT32>(FNamePool + Offsets::FNamePoolToCurrentByteCursor);
        if (block > CurrentBlock || (block == CurrentBlock && offset >= CurrentByteCursor)) {
            if (Offsets::PrintWarning) {
                cout << "Invalid index. => 0x" << std::hex << index << endl;
            }
            return "None";
        }

        uintptr_t NamePoolChunk = Read<uintptr_t>(FNamePool + Offsets::FNamePoolToBlocks + (block * Offsets::PointerSize));
        uintptr_t FNameEntry = NamePoolChunk + offset;

        UINT16 FNameEntryHeader = Read<UINT16>(FNameEntry + Offsets::FNameEntryHeader);
        uintptr_t strPtr = FNameEntry + Offsets::FNameEntryToString;
        UINT16 strLen = FNameEntryHeader >> Offsets::FNameEntryToLenBit;

        if (strLen > 0) {
            string name = ReadString(strPtr, strLen);

            if (Offsets::IsDecrypt) {
                name = XorCypher(name);
            }
            nameCacheMap[index] = name;
            return name;
        }
        else {
            if (Offsets::PrintWarning) {
                cout << "Invalid string length. index=0x" << std::hex << index << endl;
            }
            return "None";
        }

    }

    static string XorCypher(string input) {
        INT32 key = (INT32)input.size();
        string output = input;

        for (size_t i = 0; i < input.size(); i++)
            output[i] = input[i] ^ key;

        return output;
    }

};


//类的成员变量和成员函数
struct ClassMember {
    StringTable variable;
    StringTable func;
};
struct SDKInfo {
    //保存需要输出的类的指针
    list<uintptr_t> finalClass;

    //保存所有类的指针和继承路径<类的指针, list<父类名称>>
    map<uintptr_t, list<FName>> pathMap;

    //保存所有类和它的成员<类名, 成员>
    map<FName, ClassMember> classMap;
};



TArray GetActorArray()
{
    uintptr_t GWorld = Read<uintptr_t>(Offsets::ModuleBase + Offsets::GWorld);
    uintptr_t PersistentLevel = Read<uintptr_t>(GWorld + Offsets::UWorldToPersistentLevel);
    return Read<TArray>(PersistentLevel + Offsets::ULevelToAActors);
}

/******************** GUobject Decryption ********************/
INT32 GetObjectCount() {
    return Read<INT32>((Offsets::ModuleBase + Offsets::TUObjectArray) + Offsets::FUObjectArrayToTUObjectArray + Offsets::TUObjectArrayToNumElements);
}

uintptr_t GetUObjectFromID(UINT32 index) {
    uintptr_t FUObjectArray = (Offsets::ModuleBase + Offsets::TUObjectArray);

    uintptr_t TUObjectArray = Read<uintptr_t>(FUObjectArray + Offsets::FUObjectArrayToTUObjectArray);
    uintptr_t Chunk = Read<uintptr_t>(TUObjectArray + ((index / 0x10000) * Offsets::PointerSize));
    return Read<uintptr_t>(Chunk + Offsets::FUObjectItemPadd + ((index % 0x10000) * Offsets::FUObjectItemSize));
}

/******************** Unreal Engine GUObject Structure ********************/
struct UObject {
    static INT32 getIndex(uintptr_t object) {
        return Read<INT32>(object + Offsets::UObjectToInternalIndex);
    }

    static uintptr_t getClass(uintptr_t object) {//UClass*
        return Read<uintptr_t>(object + Offsets::UObjectToClassPrivate);
    }

    static FName getFName(uintptr_t object) {
        return Read<FName>(object + Offsets::UObjectToFNamePrivate);
    }

    static uintptr_t getOuter(uintptr_t object) {//UObject*
        return Read<uintptr_t>(object + Offsets::UObjectToOuterPrivate);
    }

    static string getNameString(uintptr_t object) {
        return getFName(object).GetString();
    }

    static string getClassNameString(uintptr_t object) {
        return getNameString(getClass(object));
    }

    static bool isValid(uintptr_t object) {
        return (object > 0 && getFName(object).index > 0 && getClass(object) > 0);
    }
};

/******************** Unreal Engine UField Structure ********************/
struct UField {
    static uintptr_t getNext(uintptr_t field) {//UField*
        return Read<uintptr_t>(field + Offsets::UFieldToNext);
    }
};

/******************** Unreal Engine FField Structure ********************/
struct FField {
    static string getName(uintptr_t fField) {
        return Read<FName>(fField + Offsets::FFieldToName).GetString();
    }

    static string getClassName(uintptr_t fField) {
        uintptr_t clazz = Read<uintptr_t>(fField + Offsets::FFieldToClass);
        return Read<FName>(clazz).GetString();
    }

    static uintptr_t getNext(uintptr_t fField) {//UField*
        return Read<uintptr_t>(fField + Offsets::FFieldToNext);
    }
};

/******************** Unreal Engine UStruct Structure ********************/
struct UStruct {
    static uintptr_t getSuperClass(uintptr_t structz) {//UStruct*
        return Read<uintptr_t>(structz + Offsets::UStructToSuperStruct);
    }

    static uintptr_t getChildren(uintptr_t structz) {//UField*
        return Read<uintptr_t>(structz + Offsets::UStructToChildren);
    }

    static uintptr_t getChildProperties(uintptr_t structz) {//UField*
        return Read<uintptr_t>(structz + Offsets::UStructToChildProperties);
    }

    static string getClassName(uintptr_t clazz) {
        return UObject::getNameString(clazz);
    }

    static string getClassPath(uintptr_t object) {
        uintptr_t clazz = UObject::getClass(object);
        return getStructClassPath(clazz);
    }

    static string getStructClassPath(uintptr_t clazz) {
        string className = UObject::getNameString(clazz);

        uintptr_t superclass = getSuperClass(clazz);
        while (superclass) {
            className += ".";
            className += UObject::getNameString(superclass);

            superclass = getSuperClass(superclass);
        }

        return className;
    }
};

/******************** Unreal Engine UFunction Structure ********************/
struct UFunction {
    static INT32 getFunctionFlags(uintptr_t func) {
        return Read<INT32>(func + Offsets::UFunctionToFunctionFlags);
    }

    static uintptr_t getFunc(uintptr_t func) {
        return Read<uintptr_t>(func + Offsets::UFunctionToFunc);
    }
};

/******************** Unreal Engine UProperty Structure ********************/
struct UProperty {
    static INT32 getElementSize(uintptr_t prop) {
        return Read<INT32>(prop + Offsets::UPropertyToElementSize);
    }

    static UINT64 getPropertyFlags(uintptr_t prop) {
        return Read<UINT64>(prop + Offsets::UPropertyToPropertyFlags);
    }

    static INT32 getOffset(uintptr_t prop) {
        return Read<INT32>(prop + Offsets::UPropertyToOffsetInternal);
    }
};

/******************** Unreal Engine UBoolProperty Structure ********************/
struct UBoolProperty {
    static UINT8 getFieldSize(uintptr_t prop) {
        return Read<UINT8>(prop + Offsets::UBoolPropertyToFieldSize);
    }

    static UINT8 getByteOffset(uintptr_t prop) {
        return Read<UINT8>(prop + Offsets::UBoolPropertyToByteOffset);
    }

    static UINT8 getByteMask(uintptr_t prop) {
        return Read<UINT8>(prop + Offsets::UBoolPropertyToByteMask);
    }

    static UINT8 getFieldMask(uintptr_t prop) {
        return Read<UINT8>(prop + Offsets::UBoolPropertyToFieldMask);
    }
};

/******************** Unreal Engine UObjectProperty Structure ********************/
struct UObjectProperty {
    static uintptr_t getPropertyClass(uintptr_t prop) {//class UClass*
        return Read<uintptr_t>(prop + Offsets::UObjectPropertyToPropertyClass);
    }
};

/******************** Unreal Engine UClassProperty Structure ********************/
struct UClassProperty {
    static uintptr_t getMetaClass(uintptr_t prop) {//class UClass*
        return Read<uintptr_t>(prop + Offsets::UClassPropertyToMetaClass);
    }
};

/******************** Unreal Engine UInterfaceProperty Structure ********************/
struct UInterfaceProperty {
    static uintptr_t getInterfaceClass(uintptr_t prop) {//class UClass*
        return Read<uintptr_t>(prop + Offsets::UInterfacePropertyToInterfaceClass);
    }
};

/******************** Unreal Engine UArrayProperty Structure ********************/
struct UArrayProperty {
    static uintptr_t getInner(uintptr_t prop) {//UProperty*
        return Read<uintptr_t>(prop + Offsets::UArrayPropertyToInnerProperty);
    }
};

/******************** Unreal Engine UMapProperty Structure ********************/
struct UMapProperty {
    static uintptr_t getKeyProp(uintptr_t prop) {//UProperty*
        return Read<uintptr_t>(prop + Offsets::UMapPropertyToKeyProp);
    }

    static uintptr_t getValueProp(uintptr_t prop) {//UProperty*
        return Read<uintptr_t>(prop + Offsets::UMapPropertyToValueProp);
    }
};

/******************** Unreal Engine USetProperty Structure ********************/
struct USetProperty {
    static uintptr_t getElementProp(uintptr_t prop) {//UProperty*
        return Read<uintptr_t>(prop + Offsets::USetPropertyToElementProp);
    }
};

/******************** Unreal Engine UStructProperty Structure ********************/
struct UStructProperty {
    static uintptr_t getStruct(uintptr_t prop) {//UStruct*
        return Read<uintptr_t>(prop + Offsets::UStructPropertyToStruct);
    }
};
