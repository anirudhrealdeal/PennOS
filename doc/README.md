# PennOS

## Group Members
- **Name:** [Robert Thomas Bunch]
- **Penn Key:** [tbunch]

- **Name:** [Spencer Lee]
- **Penn Key:** [spenlee]

- **Name:** [Anirudh Chakravarty Kumar]
- **Penn Key:** [kumar5]

- **Name:** [Aditya Pratap Singh]
- **Penn Key:** [aditya27]

## Submitted Source Files

### Main Executables
- `src/pennos.c` - Main PennOS entry point
- `src/pennfat.c` - Standalone PennFAT program

### I/O Subsystem (`src/io/`)

#### FAT File System (`src/io/fat/`)
- `fat_t.h` - FAT data structure definitions
- `file_t.h` - File type definitions
- `k_fat.c` / `k_fat.h` - Kernel-level FAT operations
- `s_fat.c` / `s_fat.h` - System-call level FAT operations

#### File Operations (`src/io/file/`)
- `h_file.c` / `h_file.h` - Host file operations
- `k_file.c` / `k_file.h` - Kernel-level file operations
- `s_file.c` / `s_file.h` - System-call level file operations

#### File Table (`src/io/filetable/`)
- `filetable.h` - File table type definitions
- `k_filetable.c` / `k_filetable.h` - Kernel file table management

#### Path Operations (`src/io/path/`)
- `k_path.c` / `k_path.h` - Kernel-level path handling
- `s_path.c` / `s_path.h` - System-call level path handling

#### Terminal I/O (`src/io/term/`)
- `k_term.c` / `k_term.h` - Kernel-level terminal operations
- `s_term.c` / `s_term.h` - System-call level terminal operations

#### Other I/O
- `fstd.h` - File system standard definitions

### Process Subsystem (`src/process/`)

#### Process Lifecycle (`src/process/lifecycle/`)
- `k_lifecycle.c` / `k_lifecycle.h` - Kernel process lifecycle management
- `s_lifecycle.c` / `s_lifecycle.h` - System-call level lifecycle functions
- `thread_info_t.h` - Thread information type definitions

#### Process Control Block (`src/process/pcb/`)
- `pcb_t.h` - PCB data structure
- `pcb_state_t.h` - Process state enumeration
- `pcb_table_t.h` - PCB table type definitions
- `k_pcb.c` / `k_pcb.h` - Kernel-level PCB operations
- `s_pcb.c` / `s_pcb.h` - System-call level PCB operations

#### Scheduler (`src/process/schedule/`)
- `k_schedule.c` / `k_schedule.h` - Kernel priority scheduler implementation
- `s_schedule.c` / `s_schedule.h` - System-call level scheduling functions

#### Signals (`src/process/signal/`)
- `k_signal.c` / `k_signal.h` - Kernel-level signal handling (P_SIGSTOP, P_SIGCONT, P_SIGTERM)
- `s_signal.c` / `s_signal.h` - System-call level signal functions

#### Other Process
- `pstd.h` - Process standard definitions

### Shell (`src/shell/`)
- `shell.c` / `u_shell.h` - Shell main loop and initialization
- `commands.c` / `commands.h` - Built-in command implementations (cat, ls, touch, mv, cp, rm, chmod, ps, kill, etc.)

#### Shell File Commands (`src/shell/file/`)
- `file.c` / `u_file.h` - User-level file operations

#### Job Control (`src/shell/job/`)
- `job_control.c` - Job control logic (fg, bg, jobs)
- `job_table.c` / `job_table_t.h` - Job table management
- `u_job.h` - Job control interface

#### Shell Process Commands (`src/shell/process/`)
- `process.c` / `u_process.h` - User-level process commands (zombify, orphanify, nice, etc.)

#### Demo/Stress Testing (`src/shell/demo/`)
- `demo.c` / `u_demo.h` - Demo utilities
- `stress.c` - Stress testing routines

### Utilities (`src/util/`)
- `log.c` / `log.h` - Scheduler logging facility
- `panic.c` / `panic.h` - Kernel panic handling
- `parser.c` / `parser.h` - Command-line parser
- `spthread.c` / `spthread.h` - Thread wrapper library
- `vector.h` - Generic vector data structure'

## Description of code and code layout
25fa-cis5480-pennos-40/
├── Makefile             
├── doc/
│   ├── COMPANION.md           # Detailed implementation documentation
│   └── README.md              # Original assignment README
├── src/
│   ├── pennos.c               # PennOS entry point
│   ├── pennfat.c              # Standalone PennFAT program
│   ├── io/                    # I/O subsystem
│   │   ├── fat/              # FAT filesystem implementation
│   │   ├── file/             # File operations (kernel, system, host)
│   │   ├── filetable/        # File descriptor management
│   │   ├── path/             # Path resolution
│   │   └── term/             # Terminal I/O
│   ├── process/               # Process management subsystem
│   │   ├── lifecycle/        # Process creation/termination
│   │   ├── pcb/              # Process Control Block
│   │   ├── schedule/         # Priority scheduler
│   │   └── signal/           # Signal handling
│   ├── shell/                 # Shell implementation
│   │   ├── commands.c        # Command dispatcher
│   │   ├── file/             # File commands (cat, ls, mv, etc.)
│   │   ├── job/              # Job control
│   │   ├── process/          # Process commands (ps, kill, nice)
│   │   └── demo/             # Testing utilities
│   └── util/                  # Utilities
│       ├── log.c             # Scheduler logging
│       ├── parser.c          # Command parser
│       ├── spthread.c        # Thread wrapper library
│       └── vector.h          # Generic vector
├── test/                      # Test files
└── specs.txt                  # Original specification

## Compilation Instructions

```bash
# Navigate to the project directory
cd workspace

# Build all executables (pennos and pennfat)
make all

# Clean build artifacts
make clean
