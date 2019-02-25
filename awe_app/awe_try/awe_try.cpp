// awe_try.cpp : Defines the entry point for the application.
//
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "awe_try.h"

const SIZE_T MEGA_PAGE_BYTES = 4 * 1024 * 1024;  // haw many bytes takes 1 mega pages
const SIZE_T MEMORY_REQUESTED = 1024LL * MEGA_PAGE_BYTES; // request memory in bytes

bool loggedSetLockPagesPrivilege(HANDLE hProcess, bool bEnable);

int main(int argc, char *argv[])
{
	SIZE_T allocated_size = MEMORY_REQUESTED;
	if(argc > 1) {
		if (atoi(argv[1]) <= 0){
			_tprintf(_T("Wrong argument '%s'. It must be UINT value.\n"), argv[1]);
			return -1;
		}
		allocated_size = atoi(argv[1]) * MEGA_PAGE_BYTES;
		_tprintf(_T("Try to allocate %lld bytes (%lld mega pages) required by command line argument.\n"), allocated_size, allocated_size/MEGA_PAGE_BYTES);
	} else {
		if(argc == 1) {
			_tprintf(_T("Usage: \n\t%s [number of mega pages]\n"), argv[0]);
		}
		_tprintf(_T("Try to allocate default %lld (%lld mega pages)\n"), allocated_size, allocated_size/MEGA_PAGE_BYTES);
	}

	SYSTEM_INFO sSysInfo = {};   // useful system information
	::GetSystemInfo(&sSysInfo);  // fill the system information structure

	_tprintf(_T("This computer has page size %d.\n"), sSysInfo.dwPageSize);

	// Calculate the number of pages of memory to request.

	ULONG_PTR numberOfPages = allocated_size / sSysInfo.dwPageSize; // number of pages to request
	_tprintf(_T("Requesting %I64u pages of memory.\n"), numberOfPages);

	// Calculate the size of the user PFN array.
	size_t PFNArraySize = numberOfPages * sizeof(ULONG_PTR); // memory to request for PFN array
	_tprintf(_T("Requesting a PFN array of %I64u bytes.\n"), PFNArraySize);

	// page info; holds opaque data
	auto aPFNs = reinterpret_cast<ULONG_PTR *>(::HeapAlloc(::GetProcessHeap(), 0, PFNArraySize));

	if (aPFNs == NULL)
	{
		_tprintf(_T("Failed to allocate on heap.\n"));
		return 1;
	}

	// Enable the privilege.

	if (!loggedSetLockPagesPrivilege(::GetCurrentProcess(), TRUE))
	{
		return 2;
	}

	// Allocate the physical memory.

	ULONG_PTR numberOfPagesInitial = numberOfPages; // initial number of pages requested
	BOOL bResult = ::AllocateUserPhysicalPages(::GetCurrentProcess(),
		&numberOfPages,
		aPFNs);

	if (bResult != TRUE)
	{
		DWORD dwErr = ::GetLastError();
		_tprintf(_T("Cannot allocate physical pages (%u)\n"), dwErr);
		return int(dwErr);
	}

	if (numberOfPagesInitial != numberOfPages)
	{
		_tprintf(_T("Allocated only %I64u pages.\n"), numberOfPages);
		return 3;
	}

	// Reserve the virtual memory.
	PVOID lpMemReserved = ::VirtualAlloc(NULL,
		allocated_size,
		MEM_RESERVE | MEM_PHYSICAL,
		PAGE_READWRITE);
	if (lpMemReserved == NULL)
	{
		_tprintf(_T("Cannot reserve memory.\n"));
		return 4;
	}

	// Map the physical memory into the window.

	// bResult = ::MapUserPhysicalPages(lpMemReserved,
	// 	numberOfPages,
	// 	aPFNs);
	// if (FALSE == bResult)
	// {
	// 	DWORD dwErr = ::GetLastError();
	// 	_tprintf(_T("MapUserPhysicalPages failed (%u)\n"), dwErr);
	// 	return int(dwErr);
	// }

	_tprintf(_T("Looks like everything - OK\n"));
	if(argc <= 1) {
		_tprintf(_T("\tPress enter...\n"));
		::getchar();
	}

	// unmap
	bResult = ::MapUserPhysicalPages(lpMemReserved,
		numberOfPages,
		NULL);

	if (bResult != TRUE)
	{
		DWORD dwErr = ::GetLastError();
		_tprintf(_T("MapUserPhysicalPages failed (%u)\n"), dwErr);
		return int(dwErr);
	}

	// Free the physical pages.
	bResult = FreeUserPhysicalPages(GetCurrentProcess(),
		&numberOfPages,
		aPFNs);
	if (FALSE == bResult)
	{
		DWORD dwErr = ::GetLastError();
		_tprintf(_T("Cannot free physical pages, error %u.\n"), dwErr);
		return int(dwErr);
	}

	// Free virtual memory.
	bResult = ::VirtualFree(lpMemReserved,
		0,
		MEM_RELEASE);

	// Release the aPFNs array.
	bResult = ::HeapFree(GetProcessHeap(), 0, aPFNs);

	if (FALSE == bResult)
	{
		DWORD dwErr = ::GetLastError();
		_tprintf(_T("Call to HeapFree has failed (%u)\n"), dwErr);
		return int(dwErr);
	}

	return 0;
}

/*****************************************************************
loggedSetLockPagesPrivilege: a function to obtain or
release the privilege of locking physical pages.

Inputs:

HANDLE hProcess: Handle for the process for which the
privilege is needed

bool bEnable: Enable (true) or disable?

Return value: true indicates success, false failure.

*****************************************************************/
bool loggedSetLockPagesPrivilege(HANDLE hProcess, bool bEnable)
{
	struct {
		DWORD Count;
		LUID_AND_ATTRIBUTES Privilege[1];
	} Info;

	HANDLE hToken = INVALID_HANDLE_VALUE;

	// Open the token.
	BOOL bResult = OpenProcessToken(hProcess,
		TOKEN_ADJUST_PRIVILEGES,
		&hToken);
	if (FALSE == bResult)
	{
		_tprintf(_T("Cannot open process token.\n"));
		return FALSE;
	}

	// Enable or disable?

	Info.Count = 1;
	if (bEnable)
	{
		Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		Info.Privilege[0].Attributes = 0;
	}

	// Get the LUID.

	bResult = ::LookupPrivilegeValue(NULL,
		SE_LOCK_MEMORY_NAME,
		&(Info.Privilege[0].Luid));

	if (FALSE == bResult)
	{
		_tprintf(_T("Cannot get privilege for %s.\n"), SE_LOCK_MEMORY_NAME);
		return false;
	}

	// Adjust the privilege.

	bResult = ::AdjustTokenPrivileges(hToken, FALSE,
		(PTOKEN_PRIVILEGES)&Info,
		0, NULL, NULL);

	// Check the result.

	if (FALSE == bResult)
	{
		_tprintf(_T("Cannot adjust token privileges (%u)\n"), ::GetLastError());
		return false;
	}
	else
	{
		if (::GetLastError() != ERROR_SUCCESS)
		{
			_tprintf(_T("Cannot enable the SE_LOCK_MEMORY_NAME privilege; "));
			_tprintf(_T("please check the local policy.\n"));
			return false;
		}
	}

	::CloseHandle(hToken);
	return true;
}
