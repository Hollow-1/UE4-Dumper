#pragma once

#include <list>
#include <vector>
#include <iomanip>
#include <fstream>
#include "Structures.h"
#include "StringTable.h"


/******************** Functions ********************/
bool IsStartWith(string str, const char* check) {
    return (str.find(check, 0) == 0);
}

bool IsEqual(string s1, const char* check) {
    string s2(check);
    return (s1 == s2);
}

string ToHex(INT64 value)
{
    stringstream ss;
    ss << "0x" << hex << uppercase << value;
    return ss.str();
}

string ResolvePropertyNew(list<uintptr_t>& recurrce, uintptr_t prop) {
    if (prop) {
        string cname = FField::getClassName(prop);

        if (IsEqual(cname, "ObjectProperty") || IsEqual(cname, "WeakObjectProperty")
            || IsEqual(cname, "LazyObjectProperty") || IsEqual(cname, "AssetObjectProperty")
            || IsEqual(cname, "SoftObjectProperty")) {
            uintptr_t propertyClass = UObjectProperty::getPropertyClass(prop);
            recurrce.push_back(propertyClass);
            return UObject::getNameString(propertyClass) + "*";
        }
        else if (IsEqual(cname, "ClassProperty") || IsEqual(cname, "AssetClassProperty") ||
            IsEqual(cname, "SoftClassProperty")) {
            uintptr_t metaClass = UClassProperty::getMetaClass(prop);
            recurrce.push_back(metaClass);
            return "class " + UObject::getNameString(metaClass);
        }
        else if (IsEqual(cname, "InterfaceProperty")) {
            uintptr_t interfaceClass = UInterfaceProperty::getInterfaceClass(prop);
            recurrce.push_back(interfaceClass);
            return "interface class" + UObject::getNameString(interfaceClass);
        }
        else if (IsEqual(cname, "StructProperty")) {
            uintptr_t Struct = UStructProperty::getStruct(prop);
            recurrce.push_back(Struct);
            return UObject::getNameString(Struct);
        }
        else if (IsEqual(cname, "ArrayProperty")) {
            return ResolvePropertyNew(recurrce, UArrayProperty::getInner(prop)) + "[]";
        }
        else if (IsEqual(cname, "SetProperty")) {
            return "<" + ResolvePropertyNew(recurrce, USetProperty::getElementProp(prop)) + ">";
        }
        else if (IsEqual(cname, "MapProperty")) {
            return "<" + ResolvePropertyNew(recurrce, UMapProperty::getKeyProp(prop)) + "," +
                ResolvePropertyNew(recurrce, UMapProperty::getValueProp(prop)) + ">";
        }
        else if (IsEqual(cname, "BoolProperty")) {
            return "bool";
        }
        else if (IsEqual(cname, "ByteProperty")) {
            return "byte";
        }
        else if (IsEqual(cname, "IntProperty")) {
            return "int";
        }
        else if (IsEqual(cname, "Int8Property")) {
            return "int8";
        }
        else if (IsEqual(cname, "Int16Property")) {
            return "int16";
        }
        else if (IsEqual(cname, "Int64Property")) {
            return "int64";
        }
        else if (IsEqual(cname, "UInt16Property")) {
            return "uint16";
        }
        else if (IsEqual(cname, "UINT32Property")) {
            return "UINT32";
        }
        else if (IsEqual(cname, "UINT64Property")) {
            return "UINT64";
        }
        else if (IsEqual(cname, "DoubleProperty")) {
            return "double";
        }
        else if (IsEqual(cname, "FloatProperty")) {
            return "float";
        }
        else if (IsEqual(cname, "EnumProperty")) {
            return "enum";
        }
        else if (IsEqual(cname, "StrProperty")) {
            return "FString";
        }
        else if (IsEqual(cname, "TextProperty")) {
            return "FText";
        }
        else if (IsEqual(cname, "NameProperty")) {
            return "FName";
        }
        else if (IsEqual(cname, "DelegateProperty") ||
            IsEqual(cname, "MulticastDelegateProperty")) {
            return "delegate";
        }
        else {
            return FField::getName(prop) + "(" + cname + ")";
        }
    }
    return "NULL";
}


list<uintptr_t> WriteChildStructuresNew(StringTable& table, uintptr_t childprop) {
    list<uintptr_t> recurrce;

    uintptr_t child = childprop;
    while (child) {
        uintptr_t prop = child;
        string oname = FField::getName(prop);
        string cname = FField::getClassName(prop);

        if (IsEqual(cname, "ObjectProperty") || IsEqual(cname, "WeakObjectProperty") ||
            IsEqual(cname, "LazyObjectProperty") || IsEqual(cname, "AssetObjectProperty") ||
            IsEqual(cname, "SoftObjectProperty")) {
            uintptr_t propertyClass = UObjectProperty::getPropertyClass(prop);

            table.Add({ "    " + UObject::getNameString(propertyClass) + "*", oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });

            recurrce.push_back(propertyClass);
        }
        else if (IsEqual(cname, "ClassProperty") || IsEqual(cname, "AssetClassProperty") ||
            IsEqual(cname, "SoftClassProperty")) {
            uintptr_t metaClass = UClassProperty::getMetaClass(prop);

            table.Add({ "    class " + UObject::getNameString(metaClass) + "*",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
            recurrce.push_back(metaClass);
        }
        else if (IsEqual(cname, "InterfaceProperty")) {
            uintptr_t interfaceClass = UInterfaceProperty::getInterfaceClass(prop);

            table.Add({ "    interface class " + UObject::getNameString(interfaceClass) + "*",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "StructProperty")) {
            uintptr_t Struct = UStructProperty::getStruct(prop);

            table.Add({ "    " + UObject::getNameString(Struct), oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });

            recurrce.push_back(Struct);
        }
        else if (IsEqual(cname, "ArrayProperty")) {
            table.Add({ "    " + ResolvePropertyNew(recurrce, UArrayProperty::getInner(prop)) + "[]", oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "SetProperty")) {
            table.Add({ "    <" + ResolvePropertyNew(recurrce, USetProperty::getElementProp(prop)) + ">",  oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "MapProperty")) {
            table.Add({ "    <" + ResolvePropertyNew(recurrce, UMapProperty::getKeyProp(prop)) + "," +
                ResolvePropertyNew(recurrce, UMapProperty::getValueProp(prop)) + ">",  oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "BoolProperty")) {
            table.Add({ "    bool",oname + ";",
                "//(ByteOffset:" + to_string((int)UBoolProperty::getByteOffset(prop))
                + ",ByteMask:" + to_string((int)UBoolProperty::getByteMask(prop))
                + ",FieldMask:" + to_string((int)UBoolProperty::getFieldMask(prop)) + ")"
                + "[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "ByteProperty")) {
            table.Add({ "    byte",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "IntProperty")) {
            table.Add({ "    int",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "Int8Property")) {
            table.Add({ "    int8", oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "Int16Property")) {
            table.Add({ "    int16",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "Int64Property")) {
            table.Add({ "    int64",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "UInt16Property")) {
            table.Add({ "    UINT16",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "UINT32Property")) {
            table.Add({ "    UINT32", oname + ";",
                 "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "UINT64Property")) {
            table.Add({ "    UINT64",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "DoubleProperty")) {
            table.Add({ "    double",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "FloatProperty")) {
            table.Add({ "    float",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "EnumProperty")) {
            table.Add({ "    enum",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "StrProperty")) {
            table.Add({ "    FString", oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "TextProperty")) {
            table.Add({ "    FText",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "NameProperty")) {
            table.Add({ "    FName",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "DelegateProperty") ||
            IsEqual(cname, "MulticastDelegateProperty") ||
            IsEqual(cname, "MulticastInlineDelegateProperty") ||
            IsEqual(cname, "MulticastSparseDelegateProperty")) {

            table.Add({ "    delegate",oname + ";",
            "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
            + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else if (IsEqual(cname, "XigPtrProperty")) {
            table.Add({ "    XigPtrProperty",oname + ";",
                "//[Offset:" + ToHex(UProperty::getOffset(prop)) + ","
                + "Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }
        else {
            table.Add({ "    " + cname,oname + ";",
                "//[Size: " + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }

        child = FField::getNext(child);
    }
    recurrce.sort();
    return recurrce;
}

list<uintptr_t> WriteChildStructuresNew_Func(StringTable& table, uintptr_t childprop) {
    
    list<uintptr_t> recurrce;
    uintptr_t child = childprop;
    while (child) {
        uintptr_t prop = child;
        string oname = UObject::getNameString(prop);
        string cname = UObject::getClassNameString(prop);

        if (IsStartWith(cname, "Function") || IsEqual(cname, "DelegateFunction")) {
            string returnVal = "void";
            string params = "";

            uintptr_t funcParam = UStruct::getChildProperties(prop);
            while (funcParam) {
                UINT64 PropertyFlags = UProperty::getPropertyFlags(funcParam);

                if ((PropertyFlags & 0x0000000000000400) == 0x0000000000000400) {
                    returnVal = ResolvePropertyNew(recurrce, funcParam);
                }
                else {
                    if ((PropertyFlags & 0x0000000000000100) == 0x0000000000000100) {
                        params += "out ";
                    }
                    /*if((PropertyFlags & 0x0000000008000000) == 0x0000000008000000){
                        params += "ref ";
                    }*/
                    if ((PropertyFlags & 0x0000000000000002) == 0x0000000000000002) {
                        params += "const ";
                    }
                    params += ResolvePropertyNew(recurrce, funcParam);
                    params += " ";
                    params += FField::getName(funcParam);
                    params += ",";
                }

                funcParam = FField::getNext(funcParam);
            }

            if (!params.empty()) {
                params.pop_back();
                params.pop_back();
            }

            string prefix = "";

            INT32 FunctionFlags = UFunction::getFunctionFlags(prop);

            if ((FunctionFlags & 0x00002000) == 0x00002000) {
                prefix = prefix + "static ";
            }
            /*if((FunctionFlags & 0x00000001) == 0x00000001){
                prefix = prefix +"final ";
            }
            if((FunctionFlags & 0x00020000) == 0x00020000){
                prefix = prefix +"public ";
            }
            if((FunctionFlags & 0x00040000) == 0x00040000){
                prefix = prefix +"private ";
            }
            if((FunctionFlags & 0x00080000) == 0x00080000){
                prefix = prefix +"protected ";
            }*/
            table.Add({ "    " + prefix + returnVal,
                oname + "(" + params + ");" +
                 " //" + ToHex(UFunction::getFunc(prop) - Offsets::ModuleBase)
                });
        }
        else {
            table.Add({ "    " + cname,oname + ";",
                "//[Size:" + ToHex(UProperty::getElementSize(prop)) + "]"
                });
        }

        child = UField::getNext(child);
    }
    recurrce.sort();
    return recurrce;
}


void DumpClass(SDKInfo& sdk, list<uintptr_t>& recurrce, uintptr_t clazz) {
    uintptr_t currStruct = clazz;
    while(UObject::isValid(currStruct)) {
        FName name = UObject::getFName(currStruct);
        
        //保存类的指针和继承路径
        sdk.pathMap[clazz].push_front(name);
        
        //如果这个类没有读取
        if (sdk.classMap.find(name) == sdk.classMap.end()) {
            ClassMember member;
            recurrce.merge(WriteChildStructuresNew(member.variable, UStruct::getChildProperties(currStruct)));
            recurrce.merge(WriteChildStructuresNew_Func(member.func, UStruct::getChildren(currStruct)));
            sdk.classMap[name] = member;
        }
        
        if (currStruct == clazz) {
            sdk.finalClass.push_back(currStruct);
        }
        else {
            //从finalClass中删除这个类的父类（因为输出一个类的时候，会同时输出所有父类成员，所以如果类有子类，那么就不需要再输出父类
            sdk.finalClass.remove(currStruct);
        }

        currStruct = UStruct::getSuperClass(currStruct);
    }
}


void DumpStructures(SDKInfo& sdk, uintptr_t clazz) {

    if (UObject::isValid(clazz) == false) {
        return;
    }
        
    FName name = UObject::getFName(clazz);

    //如果这个类已经读取过了
    if (sdk.classMap.find(name) != sdk.classMap.end()) {
        return;
    }
    list<uintptr_t> recurrce;
    DumpClass(sdk, recurrce, clazz);

    for (auto& newClass : recurrce) {
        DumpStructures(sdk, newClass);
    }

}