# AWE

## awe_try

The command line utility for probe to use [AWE windows mechanism](https://msdn.microsoft.com/en-us/library/windows/desktop/aa366527(v=vs.85).aspx)

### How to use awe_try

In bin folder placed prebuilt exe file awe_try.exe
After start awe_try utility tries allocate 1234564 kB memory using AWE
Iaction succeeded prints follow information to output and waits when user press {{ENTER}}:

```text
This computer has page size 4096.
Requesting 308641 pages of memory.
Requesting a PFN array of 2469128 bytes.
Looks like everything - OK
 Press enter...
```

## MemInfo

The command line unitity for print PFN database information.

### How to use MemInfo

```text
usage: meminfo [-a][-u][-c][-r][-s][-w][-f][-o PID][-p PFN][-v VA]
    -a    Dump full information about each page in the PFN database
    -u    Show summary page usage information for the system
    -c    Display detailed information about the prioritized page lists
    -r    Show valid physical memory ranges detected
    -s    Display summary information about the pages on the system
    -w    Show detailed page usage information for private working sets
    -f    Display file names associated to memory mapped pages
    -o    Display information about each page in the process' working set
    -p    Display information on the given page frame index (PFN)
    -v    Display information on the given virtual address (must use -o)
```

## awe_size

The command line unitity for print in kB size of allocated AWE memory.

## lp_size

The command line unitity for print in kB size of allocated Large-Pages memory.

## How to build from sources C++ projects

Run script build.cmd (or modify it for change CMake generator type)
_Build requariments:_
* *CMake* - in the %PATH%
* Installed MS Visual Studio
