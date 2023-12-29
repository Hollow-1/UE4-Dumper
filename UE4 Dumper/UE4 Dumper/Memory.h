#pragma once

#include "Offsets.h"
#include "Singleton.h"
#include <iostream>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <Windows.h>
#include <TlHelp32.h>
#include <vmmdll.h>

#define Memory MemoryManager::Get()

using namespace std;
/*
* ����ʵ��һ���򵥵Ļ������
* ÿ�δ��ڴ��ȡ֮ǰ�����ж���û�л��棬���û�У���ȡһ���ڴ�ҳ��4K���Ž�����
* ��������ҳ�ﵽ������������Ƴ���ɵĻ��������
* ���Ҫ��ȡ�����ݿ�Խҳ�߾࣬���ȡ���ҳ��Ȼ��ƴ������
* ��������ʱ�򣬽��Ѿ�������ڴ��Ž� freeList �Ա㸴��
*/
class MemoryManager : public Singleton<MemoryManager> {
private:
	const DWORD PAGE_SIZE = 0x1000;  //4096
	const DWORD MAX_CACHE_SIZE = 0x200; //512 * 4K ��󻺴�2MB

	std::unordered_map<ULONG_PTR, PBYTE> cache;
	std::deque<ULONG_PTR> queue;
	std::list<PBYTE> freeList;
	std::mutex threadLock;


//#define _DMA_    //����ʹ�� DMA ��Ҳ����ʹ�� windows �ṩ�ĺ���

#ifdef _DMA_

//ʹ�� DMA �������̺��ڴ�
private:
	VMM_HANDLE hVMM = NULL; //VMMDLL Handle

	bool InitPrivate()
	{
		//���ʹ�õ��� DMA �忨����Ҫ�� vmware ��Ϊ fpga
		char* args[] = { (char*)"",(char*)"-device",(char*)"vmware" };
		hVMM = VMMDLL_Initialize(3, args);

		if (hVMM == NULL)
		{
			cout << "DMA: Memory Initialize failed" << endl;
			return false;
		}
		Offsets::ProcessID = GetProcessID(Offsets::TargetProcess);
		Offsets::ModuleBase = GetBaseAddress(Offsets::ProcessID, Offsets::TargetProcess);

		return true;
	}

	DWORD GetProcessID(const char* processName)
	{
		SIZE_T pcPIDs = 0;
		VMMDLL_PidList(hVMM, NULL, &pcPIDs);
		DWORD* pPIDs = (DWORD*)malloc(pcPIDs * 4);
		VMMDLL_PidList(hVMM, pPIDs, &pcPIDs);

		DWORD pid = 0;
		if (pPIDs)
		{
			for (DWORD i = 0; i < pcPIDs; i++)
			{
				LPSTR path = VMMDLL_ProcessGetInformationString(hVMM, pPIDs[i], VMMDLL_PROCESS_INFORMATION_OPT_STRING_PATH_USER_IMAGE);
				if (!path) { continue; }
				//std::cout << path << std::endl;

				char* lastSlash = strrchr(path, '\\');
				char* exeName = lastSlash + 1;

				if (strcmp(exeName, processName) == 0)
				{
					pid = pPIDs[i];
					VMMDLL_MemFree(path);
					break;
				}
				VMMDLL_MemFree(path);
			}
		}

		free(pPIDs);
		return pid;
	}

	ULONG_PTR GetBaseAddress(DWORD pid, const char* targetProcess) {
		return VMMDLL_ProcessGetModuleBaseU(hVMM, pid, (LPSTR)targetProcess);

	}

	PBYTE ReadPage(ULONG_PTR address)
	{
		ULONG64 flags = VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_NO_PREDICTIVE_READ;
		PBYTE buffer = GetPageBuffer();
		DWORD bytesRead;

		bool result = VMMDLL_MemReadEx(hVMM, Offsets::ProcessID, address, buffer, PAGE_SIZE, &bytesRead, flags);
		if (!result || bytesRead != PAGE_SIZE)
		{
			/*
			std::cout << "Read Page Failed: p=" << Offsets::ProcessID <<
				", addr=0x" << std::hex << address <<
				", size=0x" << std::hex << PAGE_SIZE <<
				", bytesRead=0x" << std::hex << bytesRead << std::endl;
			*/
			freeList.push_back(buffer);
			return NULL;
		}
		return buffer;
	}
public:
	template<typename T> void Write(ULONG_PTR address, T& value) {
		VMMDLL_MemWrite(hVMM, Offsets::ProcessID, address, &value, sizeof(T));
	}


	~MemoryManager() {
		ClearCache();
		for (auto& ptr : freeList) {
			delete[] ptr; // ��������б��е��ڴ��
		}
		VMMDLL_CloseAll();
	}


#else

// windows �ṩ�������ڴ棬���̲����ĺ�����������û�з����ױ�������Ϸ
private:
	HANDLE handle = NULL;

	bool InitPrivate()
	{
		Offsets::ProcessID = GetProcessID(Offsets::TargetProcess);
		Offsets::ModuleBase = GetBaseAddress(Offsets::ProcessID, Offsets::TargetProcess);
		handle = ::OpenProcess(PROCESS_ALL_ACCESS, NULL, Offsets::ProcessID);//Use Kernel if u have so much problem ;)
		if (handle == NULL) {
			cout << "Memory Initialize failed" << endl;
			return false;
		}
		return true;
	}
	//խ�ַ�ת��Ϊ���ַ���ʹ�������Ҫ�ֶ��ͷ�  delete[] return
	wchar_t* ToWideChar(const char* narrowString)
	{
		if (narrowString == NULL) {
			return NULL;
		}
		size_t size = strlen(narrowString) + 1;
		size_t convertedSize = 0;
		wchar_t* wideString = new wchar_t[size];
		mbstowcs_s(&convertedSize, wideString, size, narrowString, size);
		return wideString;
	}

	DWORD GetProcessID(const char* processName)
	{
		wchar_t* wideName = ToWideChar(processName);

		DWORD pid = 0;
		DWORD threadCount = 0;
		HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);
		Process32First(hSnap, &pe);
		while (Process32Next(hSnap, &pe)) {
			if (wcscmp(pe.szExeFile, wideName) == 0) {
				if (pe.cntThreads > threadCount) {
					threadCount = pe.cntThreads;
					pid = pe.th32ProcessID;
				}
			}
		}
		::CloseHandle(hSnap);
		delete[] wideName;
		return pid;
	}
	ULONG_PTR GetBaseAddress(DWORD pid, const char* targetProcess)
	{
		wchar_t* wideName = ToWideChar(targetProcess);
		ULONG_PTR base = NULL;

		HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (hSnap != INVALID_HANDLE_VALUE) {
			MODULEENTRY32 modEntry;
			modEntry.dwSize = sizeof(modEntry);
			if (Module32First(hSnap, &modEntry)) {
				do {
					if (wcscmp(modEntry.szModule, wideName) == 0) {
						base = (ULONG_PTR)modEntry.modBaseAddr;
						break;
					}
				} while (Module32Next(hSnap, &modEntry));
			}
		}
		::CloseHandle(hSnap);
		delete[] wideName;
		return base;
	}
	PBYTE ReadPage(ULONG_PTR address)
	{
		PBYTE buffer = GetPageBuffer();
		size_t bytesRead = 0;
		bool result = ::ReadProcessMemory(handle, (LPCVOID)address, buffer, PAGE_SIZE, &bytesRead);

		if (!result || bytesRead != PAGE_SIZE)
		{
			/*
			std::cout << "Read Page Failed: p=" << Offsets::ProcessID <<
				", addr=0x" << std::hex << address <<
				", size=0x" << std::hex << PAGE_SIZE <<
				", bytesRead=0x" << std::hex << bytesRead << std::endl;
			*/
			freeList.push_back(buffer);
			return NULL;
		}
		return buffer;
	}

public:

	template<typename T> void Write(ULONG_PTR address, T& value) {
		::WriteProcessMemory(handle, (LPCVOID)address, &value, sizeof(T), NULL);
	}

	~MemoryManager() {
		ClearCache();
		for (auto& ptr : freeList) {
			delete[] ptr; // ��������б��е��ڴ��
		}
		::CloseHandle(handle);
	}

#endif


public:

	/* return	true  : success
				false : failed */
	bool Init()
	{
		if (InitPrivate() == false)
		{
			return false;
		}

		cout << "ProcessID Found at " << Offsets::ProcessID << endl;
		cout << "Base Address Found at 0x" << hex << Offsets::ModuleBase << dec << endl;
		if (Offsets::ProcessID == 0 || Offsets::ModuleBase == 0)
		{
			return false;
		}
		return true;
	}

	void ClearCache() {
		std::lock_guard<std::mutex> lock(threadLock);
		for (auto& pair : cache) {
			freeList.push_back(pair.second); // ��������ڴ����������б�
		}
		cache.clear(); // ��������е�����
		queue.clear(); // ��ն���
	}

	bool Read(ULONG_PTR address, PBYTE buffer, DWORD size, bool noCache = false) {
		ULONG_PTR start_page = address >> 12;				// �� 4096�� �õ�ҳ���
		ULONG_PTR end_page = (address + size - 1) >> 12;

		ULONG_PTR start_offset = address % PAGE_SIZE;			//��һ��ҳ��Ŀ�ʼƫ��
		ULONG_PTR end_offset = start_offset + size - ((end_page - start_page) * PAGE_SIZE);//���һ��ҳ��Ľ���ƫ��
		ULONG_PTR data_index = 0;

		{
			//��֤�̰߳�ȫ
			std::lock_guard<std::mutex> lock(threadLock);
			for (ULONG_PTR i = start_page; i <= end_page; ++i) {

				PBYTE page_data = GetPage(i, noCache);
				if (page_data == NULL) {
					memset(buffer, 0, size);
					return false;
				}

				//ƴ������
				ULONG_PTR from = (i == start_page) ? start_offset : 0;
				ULONG_PTR to = (i == end_page) ? end_offset : PAGE_SIZE;
				ULONG_PTR copy_size = to - from;
				std::memcpy(buffer + data_index, page_data + from, copy_size);
				data_index += copy_size;
			}
		}

		return true;
	}
	/*
	template<typename T> T Read(ULONG_PTR address) {
		T buffer;
		Read(address, &buffer, sizeof(T));
		return buffer;
	}
	*/



private:
	//��ȡһ��ҳ���������ж�ȡ����
	PBYTE GetPage(ULONG_PTR pageIndex, bool noCache)
	{
		//���������û��
		if (cache.find(pageIndex) == cache.end()) {
			PBYTE page_data = ReadPage(pageIndex << 12);
			if (page_data == NULL) {
				return NULL;
			}
			cache[pageIndex] = page_data;
			queue.push_back(pageIndex);
			return page_data;
		}
		//���л��棬���ҿ���ʹ�û���
		if (noCache == false) {
			return cache[pageIndex];
		}

		//���л��棬������ʹ�û���
		PBYTE new_page = ReadPage(pageIndex << 12);
		if (new_page == NULL) {
			return NULL;
		}
		PBYTE old_page = cache[pageIndex];
		cache[pageIndex] = new_page;
		freeList.push_back(old_page);

		//����������ƶ������е����
		auto it = std::find(queue.begin(), queue.end(), pageIndex);
		if (it != queue.end()) {
			queue.erase(it);
			queue.push_back(pageIndex);
		}
		return new_page;
	}

	//��ȡһ�����õ�ҳ�������������洢ҳ����
	PBYTE GetPageBuffer()
	{
		//��黺���С������ﵽ��󻺴棬�Ƴ���ɵĻ���
		while (cache.size() >= MAX_CACHE_SIZE)
		{
			ULONG_PTR key = queue.front();
			PBYTE page = cache[key];

			queue.pop_front();
			cache.erase(key);
			freeList.push_back(page);
		}

		PBYTE buffer = NULL;
		if (freeList.empty()) {
			buffer = new byte[PAGE_SIZE]; // ��������б�Ϊ�գ���������ڴ�
		}
		else {
			buffer = freeList.front(); // ��ȡ�б��еĵ�һ���ڴ��
			freeList.pop_front(); // �Ƴ��б��еĵ�һ���ڴ��
		}
		return buffer;
	}

};

template<typename T> T Read(ULONG_PTR address, bool noCache = false) {
	T buffer;
	//�����ַ�ر�С�������� ��ָ�� + ƫ��
	if (address < 0x10000)
	{
		memset(&buffer, 0, sizeof(T));
		if (Offsets::PrintWarning) {
			cout << "Read failed, address is too small => 0x" << std::hex << address << endl;
		}
		return buffer;
	}

	bool success = Memory.Read(address, (PBYTE)&buffer, sizeof(T), noCache);
	if (!success && Offsets::PrintWarning) {
		cout << "Read failed => 0x" << std::hex << address << endl;
	}
	return buffer;
}

string ReadString(ULONG_PTR address, DWORD size) {
	string name(size, '\0');
	Memory.Read(address, (PBYTE)name.data(), size * sizeof(char));
	name.shrink_to_fit();
	return name;
}
