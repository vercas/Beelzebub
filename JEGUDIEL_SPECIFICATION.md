Hydrogen Specification 0.2.1b
================================================================================

§0 About This Document
--------------------------------------------------------------------------------
This document specifies the data structures used by the Hydrogen loader, the
system state after the loader has finished, and the system capabilities that
the loader requires. The hydrogen.h header file in loader/inc/hydrogen.h and
in build/hydrogen.h after the loader has been compiled is regarded as part of
this specification.

The values and definitions in this specification are subject to change in
further versions of Hydrogen, potentially breaking downward compatibility.
This document does not guarantee correct behavior of any of the shipped or
described software components.

§1 Kernel Binary
--------------------------------------------------------------------------------
Hydrogen searches for an ELF64 kernel binary attached as a Multiboot module with
a cmdline string that begins with "kernel64", loads it into virtual memory and
jumps to its entry point after the system has been set up to a defined state.
The kernel can specify an entry point for application processors (see §6.3);
when none is given, the APs will halt on kernel entry.

The kernel binary must not be loaded to any virtual address in low memory to
avoid interference with identity mappings in the lower half of the address space.

§2 Physical Memory
--------------------------------------------------------------------------------
Hydrogen is loaded at 0x100000 (1MiB mark) by the Multiboot loader. The physical
memory from that mark to 0x108000 is occupied by Hydrogen's code and data that
can be reclaimed after the kernel has been loaded. The system and info
structures begin on 0x108000:

0x108000-0x109000: The Interrupt Descriptor Table (256 entries, 16 bytes each).<br />
0x109000-0x10A000: The Global Descriptor Table (256 entries, 16 bytes each).<br />
0x10A000-0x10B000: The boot Page Model Level 4 (PML4).<br />
0x10B000-0x10C000: The PDP for identity mapping.<br />
0x10C000-0x14C000: The 64 PDs for identity mapping.<br />
0x14C000-0x14D000: The root info table (hy_info_root_t).<br />
0x14D000-0x14E000: The memory map info table (hy_info_mmap_t).<br />
0x14E000-0x14F000: The module info table (hy_info_module_t).<br />
0x14F000-0x150000: The IO APIC info table (hy_info_ioapic_t).<br />
0x150000-0x151000: The string table.<br />
0x151000-  ...   : The CPU info table (hy_info_cpu_t).<br />

The size of the CPU info table depends on the number of CPUs that are installed
into the system. Although the placement of the info structures is static (in this
version), use the offset fields in the root info table (see §5.1) to access the
other tables.

After the info tables Hydrogen places other dynamically allocated structures, such
as the kernel code, the data loaded from the binary and the paging structures for
mapping the kernel and the multiboot modules.

The free_paddr field in the root info table contains the first freely usable
address after this block of memory. The remaining physical memory (except when
marked unavailable in the memory map) is guaranteed to be free of any important
data structure.

§3 Virtual Memory
----------------------------------------------------------------------------------
Hydrogen identity maps the first 64 GiB of physical memory using 2MB pages. The
PDP and the 64 PDs for that mapping are located in physical memory as described
in §2.

The kernel is mapped to the addresses given in its ELF64 binary. Additionally the
kernel can specify locations in virtual memory to map the stacks, the info
structures and the IDT/GDT to (see §6). All of these, the kernel binary, the stacks
the info, and the IDT/GDT must not be explicitly mapped into low memory to avoid
potential interference with identity mappings.

§4 CPU and System State
----------------------------------------------------------------------------------
### §4.1 Registers and Stack
When the CPU enters the kernel (both on the BSP and AP entry point), all general
purpose registers are cleared to zero. The stack pointer (RSP) points to the top
of the CPU's virtual stack, if a virtual stack address is specified in the kernel
header (see §6.1), or to the top of the CPU's physical stack otherwise. Each stack
is 4kiB long. The CS is 0x8 (kernel code), the DS/GS/FS/SS is 0x10 (kernel data,
see §4.3). 

### §4.2 Interrupt Descriptor Table
The Interrupt Descriptor Table is loaded when the kernel is entered with a size
of 256 entries (4kiB). Its entries are configured as interrupt gates with a
minimum DPL of 0x0 and a code segment of 0x8, i.e. kernel code (see §4.3). When
a ISR entry table is specified in the kernel header (see §6.5) the ISR addresses
for the entries are set accordingly; otherwise they are cleared to zero. When
a virtual address for the IDT is specified (see §6.9), the IDT is mapped to and
reloaded from that position.

### §4.3 Global Descriptor Table
The global descriptor table is loaded when the kernel is entered with a size
of 256 entries (4kiB). All entries except for the 2nd to 5th are cleared to zero.
The 2nd to 5th entries are configured to be code/data segments with a base of
0x0 and a limit of 0xFFFFFFFFFFFFFFFF (flat memory model) in long mode and differ
as follows:

0x08: Kernel Code (DPL 0)<br />
0x10: Kernel Data (DPL 0)<br />
0x18: User Code (DPL 3)<br />
0x20: User Data (DPL 3)<br />

When a virtual address for the GDT is specified (see §6.9), the GDT is mapped to
and reloaded from that position.

### §4.4 Syscall MSRs
The syscall related MSRs are configured as follows: The STAR register specifies
0x8 as the kernel code segment and 0x18 as the user code segment. The SFMASK
register is configured to clear the IF flag (interrupt flag) on a system call.
The CSTAR register is fixed to zero and the LSTAR register is set to the value
given in the kernel header (see §6.4).

### §4.5 LAPIC State
When Hydrogen detects that the BSP's LAPIC supports the x2APIC mode, it will
assume it is supported on all LAPICs installed into the system. When the kernel
supports the x2APIC (see §6.8) and it is present, the LAPICs are initialized in
x2APIC mode, otherwise they stay in xAPIC compatibility mode. When the kernel
requires the x2APIC, but the system does not support it, Hydrogen will panic. 

In both cases (xAPIC and x2APIC) the LAPIC of each CPU is enabled (enable bit
set in SVR) and has a spurious interrupt vector of 0x20. The LINT0 pin is
configured to ExtINT delivery with level trigger, the LINT1 pin is configured
to NMI delivery with edge trigger. The performance counter and error interrupt
is masked. The task priority register (TPR) is cleared (zero).

In xAPIC mode the the logical destination register (LDR) is set individually for
each CPU to the result of (1 << (APIC ID % 8)).

### §4.6 IO APIC State
All IO APIC redirections that correspond to an ISA IRQ are configured to be
active high edge triggered, unless stated otherwise in an Interrupt Source
Override in the APCI tables. The vector and mask depends on the IRQ
configuration in the kernel header (see §6.6). All other redirections are
configured in a PCI-like manner to be active low level triggered. Their vector
is fixed to zero and they are masked.

The delivery mode of each redirection is set to be lowest priority and the
destination is set to 0xFF in logical destination mode, meaning that IRQs
are load-balanced between all CPUs in the platform. When the kernel header
sets the HY_HEADER_FLAG_IOAPIC_BSP flag (see §6.7), the delivery mode is
fixed instead and the destination is set to the BSP's LAPIC ID in fixed
destination mode.

§5 Info Tables
----------------------------------------------------------------------------------
The info tables are located at fixed physical addresses, as described in §2, and
may be mapped to an address in virtual memory, as described in §6.2.

### §5.1 Root Info Table
The first info structure is the root table (hy_info_root_t). It contains the
length of the info tables as well as the offsets to the various sub-tables and
the number of entries in each table.

In addition it provides some general information about the system, like the
physical addresses of discovered data structures (such as the RSDP or the LAPIC
MMIO region) or IRQ properties and mappings.

### §5.2 CPU Info Table
The CPU info table is a map of CPU structures (hy_info_cpu_t) to their APIC id.
Any entry for an APIC id that corresponds to an actual processor has its
HY_INFO_CPU_FLAG_PRESENT flag set, any other entry must be ignored. While the
cpu_count field of the root info table contains the length of this table
including non-present entries, the cpu_count_active field contains the number
of present entries in this table. For each CPU the APIC id and the ACPI id
is specified in additional to some flags and the frequency of ticks in the
CPU's LAPIC timer (in Hz for an divisor of 1). Additionally the id of the
NUMA domain the CPU belongs to is given.

### §5.3 IO APIC Info Table
The IO APIC info table is a list of IO APIC structures (hy_info_ioapic_t).
Each structure corresponds to a separate IO APIC installed into the system
(typically only one). For each IO APIC the APIC id and the version is
specified, as well as the range of Global System Interrupts that are handled
by the IO APIC and the physical address of its MMIO region.

### §5.4 Memory Map
The Memory Map is a list of memory map entries (hy_info_mmap_t). Each entry
represents a region in physical memory, given its address and length. Each
entry specifies, whether the region is available or should not be used as
general purpose memory. When there is no entry that covers a byte in physical
memory, this byte should be regarded as unavailable.

### §5.5 Module Info Table
The module info table is a list of module structures (hy_info_module_t). Each
structure specifies the address and length of the module in physical memory
and contains an offset into the string table for the module's name.

### §5.6 String Table
The string table is a collection of null-terminated strings. Info tables may
specify offsets into this table, when they specify a string value.

§6 Kernel Header
----------------------------------------------------------------------------------
The kernel header (hy_header_root_t) is a structure that must be provided by the
kernel binary and that is used to configure Hydrogen's startup process. The kernel
binary must export a symbol "hydrogen_header" that points to the header structure,
or Hydrogen refuses to load.

### §6.1 Stack Mapping
The kernel header (hy_header_root_t) can specify a virtual address for mapping
the stacks into virtual memory. When a virtual address (non-null) is specified for
the stack mapping, the 4kB stacks are mapped to that address according to the APIC
id of the CPU they belong to (stack_vaddr + 0x1000 * APIC ID).

The stack pointers will be set to the top of these virtual stacks instead of the
top of the physical ones (see §4). When no virtual address (null) is specified, the
stacks will not be mapped and the stack pointers will point to the top of the
physical stacks.

Make sure the mapping of the stacks does not interfere with other mappings (such
as the kernel or the info tables) and is in high memory. Otherwise, the resulting
state is undefined.

### §6.2 Info Table Mapping
The kernel header can specify a virtual address for mapping the info tables into
virtual memory. When a virtual address (non-null) is specified, the info tables
will be mapped as they are in physical memory (see §2) to the address in virtual
memory, maintaining the same offsets. Otherwise the info tables will not be mapped.

### §6.3 AP Entry Point
The kernel header can specify an address that functions as an entry point of the
application processors into the kernel binary, in addition to the ELF64 entry for
the BSP. The BSP and the APs will synchronize on a barrier and when finished, jump
to their respective entry points. When no AP entry point is specified, the APs
will halt on kernel entry.

### §6.4 Syscall Entry Point
The kernel header can specify a system call entry point that is written into the
LSTAR register of all CPUs. See §4.4 for more syscall related information.

### §6.5 ISR Entry Table
The kernel header can specify the virtual address of an ISR entry table contained
in the kernel binary. This table contains 256 virtual addresses (8 bytes each) of
the 256 ISRs. When the pointer to the ISR entry table is not null, the entries of
the IDT will be configured to use the addresses for as ISRs (see §4.2).

### §6.6 IRQ Array
The kernel header contains an array of 16 IRQ structures (hy_header_irq_t), one
for each ISA IRQ number. In these structures, the kernel can specify interrupt
vectors and masks for each of the IRQs individually. The kernel will configure
the IO APICs accordingly (see §4.6 on IO APIC state).

### §6.7 IO APIC BSP Flag
When the kernel header sets the HY_HEADER_FLAG_IOAPIC_BSP flag, the IO APICs
will be configured to deliver their IRQs directly to the BSP instead of using
the lowest priority delivery mode for all CPUs (see §4.6 on IO APIC state). 

### §6.8 x2APIC Flags
Using the HY_HEADER_FLAG_X2APIC_ALLOW the kernel supports x2APIC mode and
Hydrogen will enable it on all LAPICs, if it is supported by the system. If
both HY_HEADER_FLAG_X2APIC_ALLOW and HY_HEADER_FLAG_X2APIC_REQUIRE are set,
Hydrogen will panic, if x2APIC mode is not supported on the system.

### §6.9 IDT and GDT Mapping
The kernel header can specify a virtual address for mapping the IDT/GDT into
virtual memory. When a virtual address (non-null) is specified, the IDT/GDT will be
mapped to the 4kB right after the given address and will be reloaded on all CPUs
from that address. Otherwise the IDT/GDT will is still accessible using the identity
mapping and is loaded from that address.

§7 System Requirements
----------------------------------------------------------------------------------
The host system must fulfill certain requirements in order to run Hydrogen:

 - One or more AMD64 compliant 64 bit CPUs
 - 2MB of RAM or more
 - xAPIC support (both LAPICs and IO APICs available)
 - ACPI support
 - 8253/8254 Programmable Interval Timer
 - A20 initialization using IO port 0x92
