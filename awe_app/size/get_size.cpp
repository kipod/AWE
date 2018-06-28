// get_size.cpp : Defines the entry point for the console application.
//
#define WIN32_NO_STATUS
#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>


#include <Windows.h>
#undef WIN32_NO_STATUS

#include <ntstatus.h>
#include <assert.h>

#include "meminfo/MemInfo.h"

#define SUPERFETCH_VERSION      45
#define SUPERFETCH_MAGIC        'kuhC'

#undef MI_GET_PFN
#define MI_GET_PFN(x)                   (PMMPFN_IDENTITY)(&g_MmPfnDatabase->PageData[(x)])

#pragma comment(lib, "ntdll")
SIZE_T g_MmPageCounts[TransitionPage + 1];
SIZE_T g_MmUseCounts[MMPFNUSE_SIZE];
SIZE_T g_MmPageUseCounts[MMPFNUSE_SIZE][TransitionPage + 1];
ULONG g_MmHighestPhysicalPageNumber = 0;
PPF_MEMORY_RANGE_INFO g_MemoryRanges = nullptr;
LIST_ENTRY g_MmProcessListHead = {};
ULONG g_MmProcessCount = 0;
LIST_ENTRY g_MmFileListHead = {};
ULONG g_MmPfnDatabaseSize = 0;
PPF_PFN_PRIO_REQUEST g_MmPfnDatabase = nullptr;
RTL_BITMAP g_MmVaBitmap = {};
RTL_BITMAP g_MmPfnBitMap = {};
PPF_PRIVSOURCE_QUERY_REQUEST g_MmPrivateSources;

void PfiBuildSuperfetchInfo(IN PSUPERFETCH_INFORMATION SuperfetchInfo, IN PVOID Buffer, IN ULONG Length, IN SUPERFETCH_INFORMATION_CLASS InfoClass) {
	SuperfetchInfo->Version = SUPERFETCH_VERSION;
	SuperfetchInfo->Magic = SUPERFETCH_MAGIC;
	SuperfetchInfo->Data = Buffer;
	SuperfetchInfo->Length = Length;
	SuperfetchInfo->InfoClass = InfoClass;
}

NTSTATUS PfiQueryMemoryRanges() {
	NTSTATUS Status;
	SUPERFETCH_INFORMATION SuperfetchInfo;
	PF_MEMORY_RANGE_INFO MemoryRangeInfo;
	ULONG ResultLength = 0;

	//
	// Memory Ranges API was added in RTM, this is Version 1
	//
	MemoryRangeInfo.Version = 1;

	//
	// Build the Superfetch Information Buffer
	//
	PfiBuildSuperfetchInfo(&SuperfetchInfo,
		&MemoryRangeInfo,
		sizeof(MemoryRangeInfo),
		SuperfetchMemoryRangesQuery);

	//
	// Query the Memory Ranges
	//
	Status = NtQuerySystemInformation(SystemSuperfetchInformation,
		&SuperfetchInfo,
		sizeof(SuperfetchInfo),
		&ResultLength);
	if (Status == STATUS_BUFFER_TOO_SMALL) {
		//
		// Reallocate memory
		//
		g_MemoryRanges = static_cast<PPF_MEMORY_RANGE_INFO>(::HeapAlloc(::GetProcessHeap(), 0, ResultLength));
		g_MemoryRanges->Version = 1;

		//
		// Rebuild the buffer
		//
		PfiBuildSuperfetchInfo(&SuperfetchInfo,
			g_MemoryRanges,
			ResultLength,
			SuperfetchMemoryRangesQuery);

		//
		// Query memory information
		//
		Status = NtQuerySystemInformation(SystemSuperfetchInformation,
			&SuperfetchInfo,
			sizeof(SuperfetchInfo),
			&ResultLength);
		if (!NT_SUCCESS(Status)) {
			printf("Failure querying memory ranges!\n");
			return Status;
		}
	}
	else {
		//
		// Use local buffer
		//
		g_MemoryRanges = &MemoryRangeInfo;
	}

	return STATUS_SUCCESS;
}

NTSTATUS PfiInitializePfnDatabase() {
	NTSTATUS Status;
	SUPERFETCH_INFORMATION SuperfetchInfo;
	ULONG ResultLength = 0;
	PMMPFN_IDENTITY Pfn1;
	ULONG PfnCount, i, k;
	ULONG PfnOffset = 0;
	ULONG BadPfn = 0;
	PVOID BitMapBuffer;
	PPF_PFN_PRIO_REQUEST PfnDbStart;
	PPHYSICAL_MEMORY_RUN Node;

    //
	// Calculate maximum amount of memory required
	//
	PfnCount = g_MmHighestPhysicalPageNumber + 1;
	g_MmPfnDatabaseSize = FIELD_OFFSET(PF_PFN_PRIO_REQUEST, PageData) +
		PfnCount * sizeof(MMPFN_IDENTITY);

	//
	// Build the PFN List Information Request
	//
	PfnDbStart = g_MmPfnDatabase = static_cast<PPF_PFN_PRIO_REQUEST>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, g_MmPfnDatabaseSize));
	g_MmPfnDatabase->Version = 1;
	g_MmPfnDatabase->RequestFlags = 1;

	//
	// Build the Superfetch Query
	//
	PfiBuildSuperfetchInfo(&SuperfetchInfo,
		g_MmPfnDatabase,
		g_MmPfnDatabaseSize,
		SuperfetchPfnQuery);

#if 1
	//
	// Initial request, assume all bits valid
	//
	for (ULONG i = 0; i < PfnCount; i++) {
		//
		// Get the PFN and write the physical page number
		//
		Pfn1 = (PMMPFN_IDENTITY)(&g_MmPfnDatabase->PageData[(i)]);
		Pfn1->PageFrameIndex = i;
	}

	//
	// Build a bitmap of pages
	//
	BitMapBuffer = ::HeapAlloc(::GetProcessHeap(), 0, PfnCount / 8);
	RtlInitializeBitMap(&g_MmPfnBitMap, static_cast<PULONG>(BitMapBuffer), PfnCount);
	RtlSetAllBits(&g_MmPfnBitMap);
	g_MmVaBitmap = g_MmPfnBitMap;
#endif


	//
	// Loop all the ranges
	//
	for (k = 0, i = 0; i < g_MemoryRanges->RangeCount; i++) {
		//
		// Print information on the range
		//
		Node = reinterpret_cast<PPHYSICAL_MEMORY_RUN>(&g_MemoryRanges->Ranges[i]);
		for (SIZE_T j = Node->BasePage; j < (Node->BasePage + Node->PageCount); j++) {
			//
			// Get the PFN and write the physical page number
			//
			Pfn1 = (PMMPFN_IDENTITY)(&g_MmPfnDatabase->PageData[(k++)]);
			Pfn1->PageFrameIndex = j;
		}
	}

	//
	// Query all valid PFNs
	//
	g_MmPfnDatabase->PfnCount = k;

	//
	// Query the PFN Database
	//
	Status = NtQuerySystemInformation(SystemSuperfetchInformation,
		&SuperfetchInfo,
		sizeof(SuperfetchInfo),
		&ResultLength);

	return Status;
}

PPF_PROCESS PfiFindProcess(IN ULONGLONG UniqueProcessKey) {
	PLIST_ENTRY NextEntry;
	PPF_PROCESS FoundProcess;

	//
	// Sign-extend the key
	//
#ifdef _WIN64
	UniqueProcessKey |= 0xFFFF000000000000;
#else
	UniqueProcessKey |= 0xFFFFFFFF00000000;
#endif

	NextEntry = g_MmProcessListHead.Flink;
	while (NextEntry != &g_MmProcessListHead) {
		FoundProcess = CONTAINING_RECORD(NextEntry, PF_PROCESS, ProcessLinks);
		if (FoundProcess->ProcessKey == UniqueProcessKey)
			return FoundProcess;

		NextEntry = NextEntry->Flink;
	}

	return nullptr;
}

NTSTATUS PfiQueryPrivateSources() {
	NTSTATUS Status;
	SUPERFETCH_INFORMATION SuperfetchInfo;
	PF_PRIVSOURCE_QUERY_REQUEST PrivateSourcesQuery = { 0 };
	ULONG ResultLength = 0;

	/* Version 2 for Beta 2, Version 3 for RTM */
	PrivateSourcesQuery.Version = PF_PRIVSOURCE_QUERY_REQUEST_VERSION; //3;

	PfiBuildSuperfetchInfo(&SuperfetchInfo,
		&PrivateSourcesQuery,
		sizeof(PrivateSourcesQuery),
		SuperfetchPrivSourceQuery);

	Status = NtQuerySystemInformation(SystemSuperfetchInformation,
		&SuperfetchInfo,
		sizeof(SuperfetchInfo),
		&ResultLength);
	if (Status == STATUS_BUFFER_TOO_SMALL) {
		g_MmPrivateSources = static_cast<PPF_PRIVSOURCE_QUERY_REQUEST>(::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ResultLength));
		g_MmPrivateSources->Version = PF_PRIVSOURCE_QUERY_REQUEST_VERSION;

		PfiBuildSuperfetchInfo(&SuperfetchInfo,
			g_MmPrivateSources,
			ResultLength,
			SuperfetchPrivSourceQuery);

		Status = NtQuerySystemInformation(SystemSuperfetchInformation,
			&SuperfetchInfo,
			sizeof(SuperfetchInfo),
			&ResultLength);
		if (!NT_SUCCESS(Status)) {
			printf("Superfetch Information Query Failed\n");
		}
	}

	//
	// Loop the private sources
	//
	const ULONG size = min(g_MmPrivateSources->InfoCount, (ResultLength - (sizeof(PPF_PRIVSOURCE_QUERY_REQUEST) - sizeof(g_MmPrivateSources->InfoArray))) / sizeof(PF_PRIVSOURCE_INFO));
	for (ULONG i = 0; i < size; i++) {
		//
		// Make sure it's a process
		//
		const auto& info = g_MmPrivateSources->InfoArray[i];
		const ULONG64 UniqueProcessKeyMask = 0xFFFF000000000000;
		if ((info.DbInfo.Type == PfsPrivateSourceProcess) && ((ULONG64(info.EProcess) & UniqueProcessKeyMask) == UniqueProcessKeyMask)) {
			//
			// Do we already know about this process?
			//
			PPF_PROCESS Process;
			CLIENT_ID ClientId;
			OBJECT_ATTRIBUTES ObjectAttributes;
			Process = PfiFindProcess(reinterpret_cast<ULONGLONG>(info.EProcess));
			if (!Process) {
				//
				// We don't, allocate it
				//
				auto numPages =info.NumberOfPrivatePages;
				Process = static_cast<PPF_PROCESS>(::HeapAlloc(::GetProcessHeap(), 0, sizeof(PF_PROCESS) +
					numPages * sizeof(ULONG)));
				InsertTailList(&g_MmProcessListHead, &Process->ProcessLinks);
				g_MmProcessCount++;

				//
				// Set it up
				//
				Process->ProcessKey = reinterpret_cast<ULONGLONG>(info.EProcess);
				strncpy_s(Process->ProcessName, info.ImageName, 16);
				Process->ProcessPfnCount = 0;
				Process->PrivatePages = static_cast<ULONG>(info.NumberOfPrivatePages);
				Process->ProcessId = reinterpret_cast<HANDLE>(static_cast<ULONGLONG>(info.DbInfo.ProcessId));
				Process->SessionId = info.SessionID;
				Process->ProcessHandle = NULL;

				//
				// Open a handle to it
				//
				InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
				ClientId.UniqueProcess = Process->ProcessId;
				ClientId.UniqueThread = 0;
				NtOpenProcess(&Process->ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttributes, &ClientId);
			}
			else {
				//
				// Process exists -- clear stats
				//
				Process->ProcessPfnCount = 0;
			}
		}
	}

	::HeapFree(::GetProcessHeap(), 0, g_MmPrivateSources);
	return Status;
}

NTSTATUS PfiQueryPfnDatabase() {
	NTSTATUS Status;
	PMMPFN_IDENTITY Pfn1;
	SUPERFETCH_INFORMATION SuperfetchInfo;
	ULONG ResultLength = 0;

	//
	// Build the Superfetch Query
	//
	PfiBuildSuperfetchInfo(&SuperfetchInfo,
		g_MmPfnDatabase,
		g_MmPfnDatabaseSize,
		SuperfetchPfnQuery);

	//
	// Query the PFN Database
	//
	Status = NtQuerySystemInformation(SystemSuperfetchInformation,
		&SuperfetchInfo,
		sizeof(SuperfetchInfo),
		&ResultLength);
	assert(Status == STATUS_SUCCESS);

	//
	// Initialize page counts
	//
	::RtlZeroMemory(g_MmPageCounts, sizeof(g_MmPageCounts));
	::RtlZeroMemory(g_MmUseCounts, sizeof(g_MmUseCounts));
	::RtlZeroMemory(g_MmPageUseCounts, sizeof(g_MmPageUseCounts));

	//
	// Loop the database
	//
	for (ULONG i = 0; i < g_MmPfnDatabase->PfnCount; i++) {
		//
		// Get the PFN
		//
		Pfn1 = MI_GET_PFN(i);

		//
		// Save the count
		//
		g_MmPageCounts[Pfn1->u1.e1.ListDescription]++;

		//
		// Save the usage
		//
		g_MmUseCounts[Pfn1->u1.e1.UseDescription]++;

		//
		// Save both
		//
		g_MmPageUseCounts[Pfn1->u1.e1.UseDescription][Pfn1->u1.e1.ListDescription]++;

		//
		// Is this a process page?
		//
		if ((Pfn1->u1.e1.UseDescription == MMPFNUSE_PROCESSPRIVATE) && (Pfn1->u1.e4.UniqueProcessKey != 0)) {
			//
			// Get the process structure
			//
			PPF_PROCESS Process;
			Process = PfiFindProcess(Pfn1->u1.e4.UniqueProcessKey);
#if 0 // takes long time on server
			if (!Process) {
				//
				// May be... The private sources changed during a query -- reload private sources
				//
				PfiQueryPrivateSources();
				Process = PfiFindProcess(Pfn1->u1.e4.UniqueProcessKey);
			}
#endif
			if (Process) {
				//
				// Add this to the process' PFN array
				//
				Process->ProcessPfns[Process->ProcessPfnCount] = i;
				if (Process->ProcessPfnCount == Process->PrivatePages) {
					//
					// Our original estimate might be off, let's allocate some more PFNs
					//
					PLIST_ENTRY PreviousEntry, NextEntry;
					PreviousEntry = Process->ProcessLinks.Blink;
					NextEntry = Process->ProcessLinks.Flink;
					Process = static_cast<PPF_PROCESS>(::HeapReAlloc(::GetProcessHeap(), 0, Process,
						sizeof(PF_PROCESS) +
						Process->PrivatePages * 2 * sizeof(ULONG)));
					Process->PrivatePages *= 2;
					PreviousEntry->Flink = NextEntry->Blink = &Process->ProcessLinks;
				}
				//
				// One more PFN
				//
				Process->ProcessPfnCount++;
			}
		}
	}

	return STATUS_SUCCESS;
}

int main() {
    //
	// First, get required privileges
	//
	BOOLEAN old;
	NTSTATUS status = RtlAdjustPrivilege(SE_PROF_SINGLE_PROCESS_PRIVILEGE, TRUE, FALSE, &old);
	status |= RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &old);
	if (!NT_SUCCESS(status)) {
		printf("Failed to get required privileges.\n");
		printf("Make sure that you are running with administrative privileges\n"
			"and that your account has the Profile Process and Debug privileges\n");
		return 1;
	}

    //
	// Get the highest physical page on the system, it's a pre-requisite
	//
	SYSTEM_BASIC_INFORMATION basicInfo;

	status = NtQuerySystemInformation(SystemBasicInformation,
		&basicInfo, sizeof(SYSTEM_BASIC_INFORMATION), nullptr);
	if (!NT_SUCCESS(status)) {
		//
		// Shouldn't really happen
		//
		printf("Failed to get maximum physical page\n");
		return 1;
	}

	//
	// Remember the highest page
	//
	g_MmHighestPhysicalPageNumber = basicInfo.HighestPhysicalPageNumber;

	//
	// Query memory ranges
	//
	status = PfiQueryMemoryRanges();
	if (!NT_SUCCESS(status)) {
		printf("Failure getting memory ranges\n");
		return 1;
	}

    //
	// Initialize process and file table
	//
	InitializeListHead(&g_MmProcessListHead);
	InitializeListHead(&g_MmFileListHead);

	//
	// Initialize the database
	//
	status = PfiInitializePfnDatabase();
	if (!NT_SUCCESS(status)) {
		printf("Failure initializing PFN database: %X\n", status);
		return 1;
	}

	status = PfiQueryPrivateSources();

	if (NT_SUCCESS(status)) {
		status = PfiQueryPfnDatabase();
		if (NT_SUCCESS(status))
		{
#if defined(AWE_APP_SHOW_LARGE_PAGES)
			printf("%lld\n", ((g_MmPageUseCounts[MMPFNUSE_LARGEPAGE][ActiveAndValid] << PAGE_SHIFT) >> 10));
#else
			printf("%lld\n", ((g_MmPageUseCounts[MMPFNUSE_AWEPAGE][ActiveAndValid] << PAGE_SHIFT) >> 10));
#endif
		}
	}

    return 0;
}