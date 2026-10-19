#include "stdafx.h"
#include "InjectTool.h"
#include "GameAssistantComm.h"

// 远程线程注入（支持挂起注入+恢复）
BOOL InjectTool::RemoteInject(DWORD processID, const CString& dllPath, BOOL bResumeAfterInject)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "打开进程失败");
        return FALSE;
    }

    // 执行注入
    BOOL bRet = RemoteInjectSuspended(hProcess, dllPath);

    // 如果需要恢复线程
    if (bRet && bResumeAfterInject)
    {
        // 获取进程主线程
        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap != INVALID_HANDLE_VALUE)
        {
            THREADENTRY32 te;
            te.dwSize = sizeof(te);
            if (Thread32First(hThreadSnap, &te))
            {
                do
                {
                    if (te.th32OwnerProcessID == processID)
                    {
                        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                        if (hThread != NULL)
                        {
                            ResumeThread(hThread); // 恢复线程
                            CloseHandle(hThread);
                            break;
                        }
                    }
                } while (Thread32Next(hThreadSnap, &te));
            }
            CloseHandle(hThreadSnap);
        }
    }

    CloseHandle(hProcess);
    return bRet;
}

// 挂起状态下的远程注入
BOOL InjectTool::RemoteInjectSuspended(HANDLE hProcess, const CString& dllPath)
{
    // 1. 分配内存
    SIZE_T pathLen = (dllPath.GetLength() + 1) * sizeof(TCHAR);
    LPVOID pRemoteMem = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT, PAGE_READWRITE);
    if (pRemoteMem == NULL)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_MEMORY_ALLOC_FAILED, "InjectTool", "VirtualAllocEx失败");
        return FALSE;
    }

    // 2. 写入DLL路径
    CString strPath = dllPath;  // 创建可修改的副本
    BOOL writeRet = WriteProcessMemory(hProcess, pRemoteMem, strPath.GetBuffer(), pathLen, NULL);
    strPath.ReleaseBuffer();
    if (!writeRet)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_DATA_SEND_FAILED, "InjectTool", "WriteProcessMemory失败");
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        return FALSE;
    }

    // 3. 获取LoadLibraryW地址
    LPVOID pLoadLib = GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "LoadLibraryW");
    if (pLoadLib == NULL)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "GetProcAddress失败");
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        return FALSE;
    }

    // 4. 创建远程线程（注入DLL）
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLib, pRemoteMem, 0, NULL);
    if (hThread == NULL)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_THREAD_CREATE_FAILED, "InjectTool", "CreateRemoteThread失败");
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        return FALSE;
    }

    // 5. 等待注入完成
    WaitForSingleObject(hThread, 5000);

    // 6. 清理资源
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    
    return TRUE;
}

// 手动映射注入
BOOL InjectTool::ManualMapInject(DWORD processID, const CString& dllPath)
{
    // 1. 打开目标进程
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: 打开进程失败");
        return FALSE;
    }

    // 2. 读取DLL文件到内存
    HANDLE hFile = CreateFile(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: 打开DLL文件失败");
        CloseHandle(hProcess);
        return FALSE;
    }

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == 0)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: DLL文件为空");
        CloseHandle(hFile);
        CloseHandle(hProcess);
        return FALSE;
    }

    BYTE* pDllData = new BYTE[dwFileSize];
    DWORD dwBytesRead = 0;
    if (!ReadFile(hFile, pDllData, dwFileSize, &dwBytesRead, NULL))
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: 读取DLL文件失败");
        delete[] pDllData;
        CloseHandle(hFile);
        CloseHandle(hProcess);
        return FALSE;
    }
    CloseHandle(hFile);

    // 3. 验证PE文件头
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pDllData;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: 无效的PE文件");
        delete[] pDllData;
        CloseHandle(hProcess);
        return FALSE;
    }

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pDllData + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: 无效的NT头");
        delete[] pDllData;
        CloseHandle(hProcess);
        return FALSE;
    }

    // 仅支持64位DLL
    if (pNtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_PROCESS_OP_FAILED, "InjectTool", "ManualMapInject: 仅支持64位DLL");
        delete[] pDllData;
        CloseHandle(hProcess);
        return FALSE;
    }

    // 4. 在目标进程中分配内存
    SIZE_T dwSizeOfImage = pNtHeaders->OptionalHeader.SizeOfImage;
    LPVOID pRemoteBase = VirtualAllocEx(hProcess, NULL, dwSizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (pRemoteBase == NULL)
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_MEMORY_ALLOC_FAILED, "InjectTool", "ManualMapInject: 分配远程内存失败");
        delete[] pDllData;
        CloseHandle(hProcess);
        return FALSE;
    }

    // 5. 映射PE头
    SIZE_T dwSizeOfHeaders = pNtHeaders->OptionalHeader.SizeOfHeaders;
    if (!WriteProcessMemory(hProcess, pRemoteBase, pDllData, dwSizeOfHeaders, NULL))
    {
        REPORT_ERROR(ERROR_LEVEL_ERROR, ERROR_DATA_SEND_FAILED, "InjectTool", "ManualMapInject: 写入PE头失败");
        VirtualFreeEx(hProcess, pRemoteBase, 0, MEM_RELEASE);
        delete[] pDllData;
        CloseHandle(hProcess);
        return FALSE;
    }

    // 6. 映射各节区
    PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
    for (WORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
    {
        if (pSectionHeader[i].SizeOfRawData == 0 || pSectionHeader[i].PointerToRawData == 0)
        {
            continue;
        }

        // 跳过不可写的节区（如资源节区如果没有内容）
        if (memcmp(pSectionHeader[i].Name, ".rsrc", 5) == 0 && pSectionHeader[i].SizeOfRawData == 0)
        {
            continue;
        }

        LPVOID pDest = (LPVOID)((BYTE*)pRemoteBase + pSectionHeader[i].VirtualAddress);
        BYTE* pSrc = pDllData + pSectionHeader[i].PointerToRawData;

        if (!WriteProcessMemory(hProcess, pDest, pSrc, pSectionHeader[i].SizeOfRawData, NULL))
        {
            REPORT_ERROR(ERROR_LEVEL_WARN, ERROR_DATA_SEND_FAILED, "InjectTool", "ManualMapInject: 写入节区失败，尝试继续");
        }
    }

    // 7. 处理重定位
    LONGLONG lDelta = (LONGLONG)((BYTE*)pRemoteBase - pNtHeaders->OptionalHeader.ImageBase);
    if (lDelta != 0)
    {
        PIMAGE_DATA_DIRECTORY pRelocDir = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (pRelocDir->VirtualAddress != 0 && pRelocDir->Size != 0)
        {
            PIMAGE_BASE_RELOCATION pReloc = (PIMAGE_BASE_RELOCATION)(pDllData + pRelocDir->VirtualAddress);
            while (pReloc->SizeOfBlock > 0)
            {
                DWORD dwCount = (pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                PWORD pRelocEntries = (PWORD)((BYTE*)pReloc + sizeof(IMAGE_BASE_RELOCATION));
                for (DWORD j = 0; j < dwCount; j++)
                {
                    if (pRelocEntries[j] == IMAGE_REL_BASED_DIR64)
                    {
                        // 读取64位地址并应用增量
                        DWORD_PTR dwRelocAddr = (DWORD_PTR)((BYTE*)pRemoteBase + pReloc->VirtualAddress + j * sizeof(WORD));
                        DWORD_PTR dwValue = 0;
                        ReadProcessMemory(hProcess, (LPVOID)dwRelocAddr, &dwValue, sizeof(DWORD_PTR), NULL);
                        dwValue += lDelta;
                        WriteProcessMemory(hProcess, (LPVOID)dwRelocAddr, &dwValue, sizeof(DWORD_PTR), NULL);
                    }
                }
                pReloc = (PIMAGE_BASE_RELOCATION)((BYTE*)pReloc + pReloc->SizeOfBlock);
            }
        }
    }

    // 8. 修复导入表
    PIMAGE_DATA_DIRECTORY pImportDir = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (pImportDir->VirtualAddress != 0 && pImportDir->Size != 0)
    {
        PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)(pDllData + pImportDir->VirtualAddress);
        while (pImportDesc->Name != 0)
        {
            // 获取DLL名称
            char* pszDllName = (char*)(pDllData + pImportDesc->Name);
            HMODULE hModule = LoadLibraryA(pszDllName);
            if (hModule == NULL)
            {
                // 尝试用LoadLibraryW
                WCHAR wDllName[MAX_PATH];
                MultiByteToWideChar(CP_ACP, 0, pszDllName, -1, wDllName, MAX_PATH);
                hModule = LoadLibraryW(wDllName);
            }

            if (hModule != NULL)
            {
                // 处理导入函数
                if (pImportDesc->OriginalFirstThunk != 0)
                {
                    PIMAGE_THUNK_DATA pOriginalThunk = (PIMAGE_THUNK_DATA)(pDllData + pImportDesc->OriginalFirstThunk);
                    PIMAGE_THUNK_DATA pFirstThunk = (PIMAGE_THUNK_DATA)((BYTE*)pRemoteBase + pImportDesc->FirstThunk);

                    for (; pOriginalThunk->u1.AddressOfData != 0; pOriginalThunk++, pFirstThunk++)
                    {
                        if (pOriginalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
                        {
                            // 按序号导入
                            WORD wOrdinal = (WORD)(pOriginalThunk->u1.Ordinal & 0xFFFF);
                            FARPROC pFunc = GetProcAddress(hModule, (LPCSTR)wOrdinal);
                            if (pFunc != NULL)
                            {
                                DWORD_PTR dwFunc = (DWORD_PTR)pFunc;
                                WriteProcessMemory(hProcess, pFirstThunk, &dwFunc, sizeof(DWORD_PTR), NULL);
                            }
                        } else
                        {
                            // 按名称导入
                            PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)(pDllData + pOriginalThunk->u1.AddressOfData);
                            FARPROC pFunc = GetProcAddress(hModule, pImportByName->Name);
                            if (pFunc != NULL)
                            {
                                DWORD_PTR dwFunc = (DWORD_PTR)pFunc;
                                WriteProcessMemory(hProcess, pFirstThunk, &dwFunc, sizeof(DWORD_PTR), NULL);
                            }
                        }
                    }
                }
            }
            pImportDesc++;
        }
    }

    // 9. 为DllMain创建远程线程
    DWORD_PTR dwEntryPoint = (DWORD_PTR)pRemoteBase + pNtHeaders->OptionalHeader.AddressOfEntryPoint;
    
    // 使用LoadLibrary方式来加载DLL（更稳定）
    // 首先分配DLL名字符串空间
    int nPathLen = WideCharToMultiByte(CP_ACP, 0, dllPath, -1, NULL, 0, NULL, NULL);
    if (nPathLen > 0)
    {
        char* pszAnsiPath = new char[nPathLen];
        WideCharToMultiByte(CP_ACP, 0, dllPath, -1, pszAnsiPath, nPathLen, NULL, NULL);

        // 在目标进程中分配DLL名称空间
        SIZE_T szPathSize = nPathLen;
        LPVOID pRemotePath = VirtualAllocEx(hProcess, NULL, szPathSize, MEM_COMMIT, PAGE_READWRITE);
        if (pRemotePath != NULL)
        {
            WriteProcessMemory(hProcess, pRemotePath, pszAnsiPath, szPathSize, NULL);
            
            // 获取LoadLibraryA地址
            LPVOID pLoadLibraryA = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
            
            // 创建远程线程加载DLL
            HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
                (LPTHREAD_START_ROUTINE)pLoadLibraryA, pRemotePath, 0, NULL);
            
            if (hRemoteThread != NULL)
            {
                WaitForSingleObject(hRemoteThread, 5000);
                CloseHandle(hRemoteThread);
            }
            
            VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        }
        delete[] pszAnsiPath;
    }

    // 10. 清理资源
    VirtualFreeEx(hProcess, pRemoteBase, 0, MEM_RELEASE);
    delete[] pDllData;
    CloseHandle(hProcess);

    REPORT_ERROR(ERROR_LEVEL_INFO, 0, "InjectTool", "ManualMapInject: 手动映射注入完成");
    return TRUE;
}
