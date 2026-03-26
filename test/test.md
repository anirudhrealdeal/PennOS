# PennOS Complete Demo Testing Guide
**Comprehensive Testing Commands with Expected Outputs**

---

## SETUP - Run These First

```bash
# Create test files for shell demos
(yes "abcXYZ\n" | head -c 127; echo) > d1
(head -c 255 /etc/legal; echo) > d2
(head -c 127 /dev/random; echo) > d3

# Create test files for FAT demos
head -c 256 /dev/urandom > demo2
yes a | head -c 32000 > demo3
head -c 256 /etc/legal > demo9
yes b | head -c 31488 > demo11
```

---

# PART 1: KERNEL DEMOS

## Demo 1: Zombie Process

```bash
$ ./bin/pennos <fatfs>
$ zombify &
[1] 3
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   1  R   zombify
  4    3   1  Z   zombie_child
  5    2   1  R   ps
```

```bash
$ logout
```

**What to Verify:**
- zombie_child shows status 'Z' (zombie)
- Parent process (zombify) is still running

---

## Demo 2: Zombie to Orphan

```bash
$ ./bin/pennos <fatfs>
$ zombify
^C
$
```

**Expected Log Entries:**
```
[ XX]  ZOMBIE       4   1  zombie_child
[ XX]  SCHEDULE     3   1  zombify
[ XX]  SIGNALED     3   1  zombify
[ XX]  ZOMBIE       3   1  zombify
[ XX]  ORPHAN       4   1  zombie_child
[ XX]  SCHEDULE     2   0  shell
[ XX]  WAITED       3   1  zombify
[ XX]  SCHEDULE     1   0  init
[ XX]  WAITED       4   1  zombie_child
```

```bash
$ logout
```

**What to Verify:**
- Check log file for sequence: ZOMBIE → SIGNALED → ORPHAN → WAITED by init
- zombie_child becomes orphan after zombify is killed

---

## Demo 3: Orphan Process

```bash
$ ./bin/pennos <fatfs>
$ orphanify
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  4    1   1  R   orphan_child  # Note: PPID is now 1 (init)
  5    2   1  R   ps
```

```bash
$ kill -term 4
```

**Expected Log Entries:**
```
[ XX]  ZOMBIE       3   1  orphanify
[ XX]  SCHEDULE     2   0  shell
[ XX]  WAITED       3   1  orphanify
[ XX]  ORPHAN       4   1  orphan_child
```

```bash
$ logout
```

**What to Verify:**
- orphan_child has PPID of 1 (init) after orphanify exits
- init cleans up orphan_child

---

## Demo 4: Blocking Wait Stress Test (hang)

```bash
$ ./bin/pennos <fatfs>
$ hang
```

**Expected Output:**
```
child_0 was spawned
child_1 was spawned
child_2 was spawned
child_3 was spawned
child_4 was spawned
child_5 was spawned
child_6 was spawned
child_7 was spawned
child_8 was spawned
child_9 was spawned
child_1 was reaped
child_2 was reaped
child_3 was reaped
child_4 was reaped
child_5 was reaped
child_6 was reaped
child_7 was reaped
child_8 was reaped
child_0 was reaped
child_9 was reaped
$
```

```bash
$ logout
```

**What to Verify:**
- All 10 children are spawned
- All 10 children are reaped (order may vary - non-deterministic)
- Parent blocks until children complete

---

## Demo 5: Nonblocking Wait Stress Test (nohang)

```bash
$ ./bin/pennos <fatfs>
$ nohang
```

**Expected Output:**
```
child_0 was spawned
child_1 was spawned
child_2 was spawned
child_3 was spawned
child_4 was spawned
child_5 was spawned
child_6 was spawned
child_7 was spawned
child_8 was spawned
child_9 was spawned
child_0 was reaped
child_1 was reaped
child_2 was reaped
child_3 was reaped
child_4 was reaped
child_6 was reaped
child_7 was reaped
child_8 was reaped
child_5 was reaped
child_9 was reaped
$
```

```bash
$ logout
```

**What to Verify:**
- All 10 children are spawned
- All 10 children are reaped (order may vary - non-deterministic)
- Parent uses non-blocking wait

---

## Demo 6: Recursive Spawn Stress Test (recur)

```bash
$ ./bin/pennos <fatfs>
$ recur
```

**Expected Output:**
```
Gen_A was spawned
Gen_B was spawned
Gen_C was spawned
Gen_D was spawned
...
Gen_W was spawned
Gen_X was spawned
Gen_Y was spawned
Gen_Z was spawned
Gen_Z was reaped
Gen_Y was reaped
Gen_X was reaped
Gen_W was reaped
...
Gen_D was reaped
Gen_C was reaped
Gen_B was reaped
Gen_A was reaped
$
```

```bash
$ logout
```

**What to Verify:**
- 26 processes spawned (Gen_A through Gen_Z) in alphabetical order
- 26 processes reaped in reverse alphabetical order
- Each child waits for its own child before exiting

---

## Scheduling Demo 0: Single Busy Process (100% CPU)

```bash
$ ./bin/pennos <fatfs>
$ busy
```

**In Another Terminal:**
```bash
$ top | grep <your_username>
```

**Expected Output:**
```
<userid>  <pid>  100.0  ...  pennos
```

**What to Verify:**
- CPU usage is 100% (busy process running continuously)

```bash
^C
$ logout
```

---

## Scheduling Demo 1: Background Busy Priority 0 (50% CPU)

```bash
$ ./bin/pennos <fatfs>
$ busy &
[1] 3
$
```

**In Another Terminal:**
```bash
$ top | grep <your_username>
```

**Expected Output:**
```
<userid>  <pid>  50.0  ...  pennos
```

**What to Verify:**
- CPU usage is ~50% (shell priority 0 + busy priority 0, splitting equally)

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   0  R   busy
  4    2   1  R   ps
```

```bash
$ kill -term 3
$ logout
```

---

## Scheduling Demo 2: Background Busy Priority 1 (40% CPU)

```bash
$ ./bin/pennos <fatfs>
$ nice 1 busy &
[1] 3
$
```

**In Another Terminal:**
```bash
$ top | grep <your_username>
```

**Expected Output:**
```
<userid>  <pid>  40.0  ...  pennos
```

**What to Verify:**
- CPU usage is ~40%
- Priority 0 (shell) runs 1.5x more than priority 1 (busy)
- 60 quanta for shell, 40 for busy out of 100 total

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   1  R   busy
  4    2   1  R   ps
```

```bash
$ kill -term 3
$ logout
```

---

## Scheduling Demo 3: Background Busy Priority 2 (~31% CPU)

```bash
$ ./bin/pennos <fatfs>
$ nice 2 busy &
[1] 3
$
```

**In Another Terminal:**
```bash
$ top | grep <your_username>
```

**Expected Output:**
```
<userid>  <pid>  31.0  ...  pennos
```

**What to Verify:**
- CPU usage is ~31%
- Priority 0 runs 2.25x more than priority 2
- Shell gets ~69%, busy gets ~31%

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   2  R   busy
  4    2   1  R   ps
```

```bash
$ kill -term 3
$ logout
```

---

## Scheduling Demo 4: Sleep in Foreground (0% CPU)

```bash
$ ./bin/pennos <fatfs>
$ sleep 200
```

**In Another Terminal:**
```bash
$ top | grep <your_username>
```

**Expected Output:**
```
<userid>  <pid>  0.0  ...  pennos
```

**What to Verify:**
- CPU usage is 0% (all processes blocked, scheduler idling)
- Shell is blocked waiting for sleep
- Sleep is blocked for 200 ticks

```bash
^C
$ logout
```

---

## Scheduling Demo 5: Multiple Priorities - No Starvation (~76% CPU)

```bash
$ ./bin/pennos <fatfs>
$ nice 0 busy &
[1] 3
$ nice 1 busy &
[2] 4
$ nice 1 busy &
[3] 5
$ nice 2 busy &
[4] 6
$ nice 2 busy &
[5] 7
$
```

**In Another Terminal:**
```bash
$ top | grep <your_username>
```

**Expected Output:**
```
<userid>  <pid>  76.0  ...  pennos
```

**What to Verify:**
- Total CPU ~76%
- Priority 0 processes (shell + 1 busy): 60% of quanta
- Priority 1 processes (2 busy): 26.67% of quanta  
- Priority 2 processes (2 busy): 13.33% of quanta
- No starvation - all processes get scheduled

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   0  R   busy
  4    2   1  R   busy
  5    2   1  R   busy
  6    2   2  R   busy
  7    2   2  R   busy
  8    2   1  R   ps
```

**Check Log File:**
```bash
$ cat log | grep SCHEDULE | tail -100
```

**What to Verify in Log:**
- All busy processes appear in SCHEDULE entries
- No process is starved (all get scheduled regularly)
- Proper priority ratios maintained

```bash
$ kill -term 3 4 5 6 7
$ logout
```

---

# PART 2: FILE SYSTEM DEMOS (pennfat standalone)

## Demo 1: Format Filesystems

```bash
$ ./bin/pennfat
pennfat# mkfs minfs 1 0
pennfat# mkfs maxfs 32 4
pennfat# ^D
```

**Verify in Shell:**
```bash
$ ls -l minfs maxfs
```

**Expected Output:**
```
-rw-r--r-- 1 <user> <group>  268558336 <date> <time> maxfs
-rw-r--r-- 1 <user> <group>      32768 <date> <time> minfs
```

**Check minfs Structure:**
```bash
$ hd minfs
```

**Expected Output:**
```
00000000  00 01 ff ff 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00008000
```

**Check maxfs Structure:**
```bash
$ hd maxfs | head -5
```

**Expected Output:**
```
00000000  04 20 ff ff 00 00 00 00  00 00 00 00 00 00 00 00  |. ..............|
00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
```

**What to Verify:**
- minfs: 32768 bytes (256 byte blocks, 1 FAT block)
- maxfs: 268558336 bytes (4096 byte blocks, 32 FAT blocks)
- FAT[0] = 0x0001 for minfs (LSB=0, MSB=1)
- FAT[0] = 0x2004 for maxfs (LSB=4, MSB=32)
- FAT[1] = 0xFFFF (root directory block)

---

## Demo 2: One Block File

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cp -h demo2 f1
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 256 <date> <time> f1
```

```bash
pennfat# cp f1 -h demo2copy
pennfat# ^D
```

**Verify Files Match:**
```bash
$ cmp demo2 demo2copy
```

**Expected Output:**
```
(no output = files are identical)
```

**Check Filesystem Structure:**
```bash
$ hd minfs | head -40
```

**Expected Output:**
```
00000000  00 01 ff ff ff ff 00 00  00 00 00 00 00 00 00 00  |................|
00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000100  66 31 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f1..............|
00000110  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000120  00 01 00 00 02 00 01 06  <timestamp>              |................|
00000130  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000200  <random data from demo2 file>
```

**What to Verify:**
- FAT[0] = 0x01FF (first entry)
- FAT[1] = 0xFFFF (root directory, last block)
- FAT[2] = 0xFFFF (f1's single block, last block)
- Directory entry at 0x100: name="f1", size=256, first_block=2, type=1, perm=6

---

## Demo 3: Max Out Filesystem

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cp -h demo3 f2
pennfat# ls
```

**Expected Output:**
```
  2 -rw-  256 <date> <time> f1
  3 -rw- 32000 <date> <time> f2
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -50
```

**Expected Output:**
```
00000000  00 01 ff ff ff ff 04 00  05 00 06 00 07 00 08 00  |................|
00000010  09 00 0a 00 0b 00 0c 00  0d 00 0e 00 0f 00 10 00  |................|
00000020  11 00 12 00 13 00 14 00  15 00 16 00 17 00 18 00  |................|
...
000000f0  79 00 7a 00 7b 00 7c 00  7d 00 7e 00 7f 00 ff ff  |y.z.{.|.}.~.....|
00000100  66 31 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f1..............|
00000110  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000120  00 01 00 00 02 00 01 06  <timestamp>              |................|
00000130  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000140  66 32 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f2..............|
00000150  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000160  00 7d 00 00 03 00 01 06  <timestamp>              |.}..............|
```

**What to Verify:**
- FAT shows linked list: 3→4→5→...→127→0xFFFF (125 blocks for f2)
- f2 uses blocks 3-127 (125 blocks × 256 bytes = 32000 bytes)
- f1 still in block 2

---

## Demo 4: Overwrite File

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cat -w f2
hello
^D
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 256 <date> <time> f1
  3 -rw-   6 <date> <time> f2
```

```bash
pennfat# cat f2
```

**Expected Output:**
```
hello
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -50
```

**Expected Output:**
```
00000000  00 01 ff ff ff ff ff ff  00 00 00 00 00 00 00 00  |................|
00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000140  66 32 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f2..............|
00000150  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000160  06 00 00 00 03 00 01 06  <new_timestamp>          |................|
...
00000300  68 65 6c 6c 6f 0a 61 0a  61 0a 61 0a 61 0a 61 0a  |hello.a.a.a.a.a.|
```

**What to Verify:**
- FAT[3] = 0xFFFF (f2 now only uses one block)
- FAT[4-127] = 0x0000 (freed blocks)
- f2 directory entry: size=6
- Block 3 contains "hello\n"

---

## Demo 5: Expand Directory

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# touch f3 f4 f5
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 256 <date> <time> f1
  3 -rw-   6 <date> <time> f2
    -rw-   0 <date> <time> f3
    -rw-   0 <date> <time> f4
    -rw-   0 <date> <time> f5
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -60
```

**Expected Output:**
```
00000000  00 01 04 00 ff ff ff ff  ff ff 00 00 00 00 00 00  |................|
...
00000100  66 31 ...
00000140  66 32 ...
00000180  66 33 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f3..............|
00000190  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000001a0  00 00 00 00 00 00 01 06  <timestamp>              |................|
...
000001c0  66 34 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f4..............|
...
00000400  66 35 00 00 ...                                    |f5..............|
```

**What to Verify:**
- FAT[1] = 0x0004 (root directory now has 2 blocks)
- FAT[4] = 0xFFFF (second directory block)
- Directory entries for f3, f4 span across blocks 1 and 4
- f5 entry is in block 4

---

## Demo 6: Append Without New Block

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cat -a f2
hi
^D
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 256 <date> <time> f1
  3 -rw-   9 <date> <time> f2
    -rw-   0 <date> <time> f3
    -rw-   0 <date> <time> f4
    -rw-   0 <date> <time> f5
```

```bash
pennfat# cat f2
```

**Expected Output:**
```
hello
hi
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | grep -A2 "^00000300"
```

**Expected Output:**
```
00000300  68 65 6c 6c 6f 0a 68 69  0a 0a 61 0a 61 0a 61 0a  |hello.hi..a.a.a.|
```

**What to Verify:**
- f2 still uses only block 3 (fits in one block)
- Size increased from 6 to 9
- Content is "hello\nhi\n"

---

## Demo 7: Remove File

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# rm f1
pennfat# ls
```

**Expected Output:**
```
  3 -rw-   9 <date> <time> f2
    -rw-   0 <date> <time> f3
    -rw-   0 <date> <time> f4
    -rw-   0 <date> <time> f5
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -40
```

**Expected Output:**
```
00000000  00 01 04 00 00 00 ff ff  ff ff 00 00 00 00 00 00  |................|
...
00000100  01 31 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |.1..............|
00000110  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000120  00 01 00 00 02 00 01 06  <old_timestamp>          |................|
```

**What to Verify:**
- FAT[2] = 0x0000 (block 2 freed)
- Directory entry for f1: name[0] = 0x01 (deleted marker)
- f1's data still in block 2 (not required to zero out)

---

## Demo 8: File Not Found Errors

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cat f1
```

**Expected Output:**
```
f1: cannot open the file
```

```bash
pennfat# cp f1 f2
```

**Expected Output:**
```
f1: cannot open the file
```

```bash
pennfat# mv f1 f2
```

**Expected Output:**
```
mv: cannot rename f1 to f2
```

```bash
pennfat# rm f1
```

**Expected Output:**
```
rm: cannot remove f1
```

```bash
pennfat# ^D
```

**What to Verify:**
- All operations properly detect non-existent file
- Appropriate error messages displayed

---

## Demo 9: Append With New Blocks

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cp -h demo9 f1
pennfat# cat f1 -a f2
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 256 <date> <time> f1
  3 -rw- 265 <date> <time> f2
    -rw-   0 <date> <time> f3
    -rw-   0 <date> <time> f4
    -rw-   0 <date> <time> f5
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -80
```

**Expected Output:**
```
00000000  00 01 04 00 ff ff 05 00  ff ff ff ff 00 00 00 00  |................|
...
00000160  09 01 00 00 03 00 01 06  <timestamp>              |................|
...
00000300  68 65 6c 6c 6f 0a 68 69  0a 0a 54 68 65 20 70 72  |hello.hi..The pr|
...
00000500  62 79 0a 61 70 70 6c 69  63 0a 61 0a 61 0a 61 0a  |by.applic.a.a.a.|
```

**What to Verify:**
- f2 now uses 2 blocks (block 3 → block 5)
- FAT[3] = 0x0005
- FAT[5] = 0xFFFF
- Size = 265 bytes (9 from before + 256 from f1)
- Content spans two blocks

---

## Demo 10: Move Files (Rename)

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# mv f1 f5
pennfat# mv f2 f5
pennfat# ls
```

**Expected Output:**
```
  3 -rw- 265 <date> <time> f5
    -rw-   0 <date> <time> f3
    -rw-   0 <date> <time> f4
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -80
```

**Expected Output:**
```
00000000  00 01 04 00 00 00 05 00  ff ff ff ff 00 00 00 00  |................|
...
00000100  01 35 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |.5..............|
...
00000140  66 35 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f5..............|
00000150  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000160  09 01 00 00 03 00 01 06  <timestamp>              |................|
```

**What to Verify:**
- f1 entry marked deleted (0x01 or 0x15 as first byte)
- f2 renamed to f5 (overwrote the previous f5)
- f5 has f2's data (265 bytes, blocks 3→5)
- FAT[2] = 0x0000 (f1's block freed)

---

## Demo 11: Max Out Non-Contiguous

```bash
$ ./bin/pennfat
pennfat# mount minfs
pennfat# cp -h demo11 f1
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 31488 <date> <time> f1
  3 -rw-   265 <date> <time> f5
    -rw-     0 <date> <time> f3
    -rw-     0 <date> <time> f4
```

```bash
pennfat# ^D
```

**Check Filesystem:**
```bash
$ hd minfs | head -100
```

**Expected Output:**
```
00000000  00 01 04 00 06 00 05 00  ff ff ff ff 07 00 08 00  |................|
00000010  09 00 0a 00 0b 00 0c 00  0d 00 0e 00 0f 00 10 00  |................|
00000020  11 00 12 00 13 00 14 00  15 00 16 00 17 00 18 00  |................|
...
000000f0  79 00 7a 00 7b 00 7c 00  7d 00 7e 00 7f 00 ff ff  |y.z.{.|.}.~.....|
00000100  66 31 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f1..............|
00000110  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000120  00 7b 00 00 02 00 01 06  <timestamp>              |.{..............|
...
00000200  62 0a 62 0a 62 0a 62 0a  62 0a 62 0a 62 0a 62 0a  |b.b.b.b.b.b.b.b.|
```

**What to Verify:**
- f1 uses 123 blocks (31488 / 256 = 122.875 → 123 blocks)
- FAT shows: 2→6→7→8→...→127→0xFFFF
- Block 3-5 still used by f5 (non-contiguous allocation)
- File data contains "b\nb\nb\n..." pattern

---

## Demo 12: Mount/Unmount Different Filesystems

```bash
$ ./bin/pennfat
pennfat# mount maxfs
pennfat# cp -h demo3 f1
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 32000 <date> <time> f1
```

```bash
pennfat# unmount
pennfat# mount minfs
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 31488 <date> <time> f1
  3 -rw-   265 <date> <time> f5
    -rw-     0 <date> <time> f3
    -rw-     0 <date> <time> f4
```

```bash
pennfat# ^D
```

**Check maxfs:**
```bash
$ hd maxfs | head -50
```

**Expected Output:**
```
00000000  04 20 ff ff 03 00 04 00  05 00 06 00 07 00 08 00  |. ..............|
00000010  09 00 ff ff 00 00 00 00  00 00 00 00 00 00 00 00  |................|
...
00020000  66 31 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |f1..............|
00020010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00020020  00 7d 00 00 02 00 01 06  <timestamp>              |.}..............|
...
00021000  61 0a 61 0a 61 0a 61 0a  61 0a 61 0a 61 0a 61 0a  |a.a.a.a.a.a.a.a.|
```

**What to Verify:**
- maxfs has its own f1 with 32000 bytes
- minfs unchanged (separate filesystem)
- Proper unmounting saves FAT to disk

---

# PART 3: SHELL INTEGRATION DEMOS

## Setup: Create midfs Filesystem

```bash
$ ./bin/pennfat
pennfat# mkfs midfs 16 3
pennfat# mount midfs
pennfat# cp -h d1 d1
pennfat# cp -h d2 d2
pennfat# cp -h d3 d3
pennfat# ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
```

```bash
pennfat# ^D
```

**Verify:**
```bash
$ hd midfs | head -100
```

**Expected Output:**
```
00000000  03 10 ff ff ff ff ff ff  ff ff 00 00 00 00 00 00  |................|
...
00008000  64 31 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |d1..............|
00008010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00008020  80 00 00 00 02 00 01 06  <timestamp>              |................|
...
00008040  64 32 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |d2..............|
...
00008060  00 01 00 00 03 00 01 06  <timestamp>              |................|
...
00008080  64 33 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |d3..............|
...
000080a0  80 00 00 00 04 00 01 06  <timestamp>              |................|
...
00008800  61 62 63 58 59 5a 5c 6e  0a 61 62 63 58 59 5a 5c  |abcXYZ\n.abcXYZ\|
...
00009000  0a 54 68 65 20 70 72 6f  67 72 61 6d 73 20 69 6e  |.The programs in|
```

---

## Demo 1 Part 1: Simple Output Redirection

```bash
$ ./bin/pennos midfs
$ cat d1 d2
```

**Expected Output:**
```
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
a

The programs included with the Ubuntu system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Ubuntu comes with ABSOLUTELY NO WARRANTY, to the extent permitted by
appli
```

```bash
$ cat d1 d2 > d4
$ cat d4
```

**Expected Output:**
```
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
a

The programs included with the Ubuntu system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Ubuntu comes with ABSOLUTELY NO WARRANTY, to the extent permitted by
appli
```

```bash
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> d4
```

```bash
$ logout
```

**Verify Filesystem:**
```bash
$ hd midfs | grep "^00008"
```

**Expected Output:**
```
00008000  64 31 ...
00008040  64 32 ...
00008080  64 33 ...
000080c0  64 34 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |d4..............|
...
000080e0  80 01 00 00 05 00 01 06  <timestamp>              |................|
```

**What to Verify:**
- d4 created with 384 bytes (128 + 256)
- Contents match concatenation of d1 and d2
- Uses 2 blocks (block 5)

---

## Demo 1 Part 2: Append Redirection

```bash
$ ./bin/pennos midfs
$ cp d4 d5
$ cat < d1 >> d5
$ cat d5
```

**Expected Output:**
```
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
a

The programs included with the Ubuntu system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Ubuntu comes with ABSOLUTELY NO WARRANTY, to the extent permitted by
appli
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
a
```

```bash
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> d4
  6 -rw- 512 <date> <time> d5
```

```bash
$ logout
```

**What to Verify:**
- d5 has 512 bytes (384 + 128)
- Content is d4 + d1
- Append didn't truncate original content

---

## Demo 1 Part 3: Redirection Edge Cases

```bash
$ ./bin/pennos midfs
$ cat < d5 > d5
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> d4
  6 -rw-   0 <date> <time> d5
```

```bash
$ cat < d2 >> d2
```

**Expected Output:**
```
cat: input and output cannot be the same when appending
```

```bash
$ logout
```

**What to Verify:**
- Reading and writing same file with `>` truncates to 0
- Reading and writing same file with `>>` produces error
- Error prevents data corruption

---

## Demo 2: Foreground/Background Job Control (fg)

```bash
$ ./bin/pennos midfs
$ sleep 10
```

**(Immediately press Ctrl-Z)**

**Expected Output:**
```
^Z
[1]+ Stopped                 sleep 10
$
```

```bash
$ jobs
```

**Expected Output:**
```
[1]+ Stopped                 sleep 10
```

```bash
$ fg
```

**Expected Output:**
```
sleep 10
```

**(Wait for sleep to complete or press Ctrl-Z again)**

**Expected Output (after completion):**
```
[1]+ Done                    sleep 10
$
```

```bash
$ logout
```

**What to Verify:**
- Ctrl-Z sends P_SIGSTOP to foreground process
- `jobs` shows stopped job
- `fg` brings job back to foreground
- Sleep completes or can be stopped again

---

## Demo 3: Background Job Resume (bg)

```bash
$ ./bin/pennos midfs
$ sleep 10
^Z
```

**Expected Output:**
```
[1]+ Stopped                 sleep 10
$
```

```bash
$ jobs
```

**Expected Output:**
```
[1]+ Stopped                 sleep 10
```

```bash
$ bg
```

**Expected Output:**
```
[1]+ sleep 10 &
$
```

```bash
$ jobs
```

**Expected Output:**
```
[1]+ Running                 sleep 10 &
```

**(Wait for completion)**

**Expected Output:**
```
[1]+ Done                    sleep 10
$
```

```bash
$ logout
```

**What to Verify:**
- `bg` sends P_SIGCONT and moves job to background
- Job continues running in background
- Shell remains responsive
- Job completes and reports "Done"

---

## Demo 4: Foreground Sleep with Logging

```bash
$ ./bin/pennos midfs log_test
$ sleep 5
```

**(Wait 5 seconds for completion)**

**Expected Output:**
```
$
```

```bash
$ logout
```

**Check Log File:**
```bash
$ cat log_test
```

**Expected Log Entries:**
```
[ XX]  CREATE       3   1  sleep
[ XX]  BLOCKED      2   0  shell
[ XX]  SCHEDULE     3   1  sleep
[ XX]  BLOCKED      3   1  sleep
[ XX]  SCHEDULE     1   0  init
[ XX]  SCHEDULE     1   0  init
...
(50 ticks later, assuming 100ms per tick = 5 seconds)
[ XX]  UNBLOCKED    3   1  sleep
[ XX]  SCHEDULE     3   1  sleep
[ XX]  EXITED       3   1  sleep
[ XX]  ZOMBIE       3   1  sleep
[ XX]  UNBLOCKED    2   0  shell
[ XX]  SCHEDULE     2   0  shell
[ XX]  WAITED       3   1  sleep
```

**What to Verify:**
- sleep process created with priority 1
- Shell becomes blocked waiting for sleep
- Sleep blocks for ~50 ticks (5 seconds / 100ms)
- Init scheduled while both shell and sleep blocked
- Sleep unblocks, exits, becomes zombie
- Shell reaps zombie child

---

## Demo 5: Multiple Background Sleep Jobs

```bash
$ ./bin/pennos midfs
$ sleep 9 &
```

**Expected Output:**
```
[1] 3
$
```

```bash
$ sleep 8 &
```

**Expected Output:**
```
[2] 4
$
```

```bash
$ sleep 7 &
```

**Expected Output:**
```
[3] 5
$
```

```bash
$ jobs
```

**Expected Output:**
```
[1]   Running                 sleep 9 &
[2]-  Running                 sleep 8 &
[3]+ Running                 sleep 7 &
```

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   1  S   sleep
  4    2   1  S   sleep
  5    2   1  S   sleep
  6    2   1  R   ps
```

**(Wait ~10 seconds for all to complete)**

**Expected Output:**
```
[3]+ Done                    sleep 7
[2]- Done                    sleep 8
[1]  Done                    sleep 9
$
```

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  7    2   1  R   ps
```

```bash
$ logout
```

**Check Log File:**
```bash
$ cat log | grep -E "CREATE|EXITED|ZOMBIE|WAITED" | grep sleep
```

**Expected Log Pattern:**
```
[ XX]  CREATE       3   1  sleep
[ XX]  CREATE       4   1  sleep
[ XX]  CREATE       5   1  sleep
...
[ XX]  EXITED       5   1  sleep
[ XX]  ZOMBIE       5   1  sleep
[ XX]  WAITED       5   1  sleep
[ XX]  EXITED       4   1  sleep
[ XX]  ZOMBIE       4   1  sleep
[ XX]  WAITED       4   1  sleep
[ XX]  EXITED       3   1  sleep
[ XX]  ZOMBIE       3   1  sleep
[ XX]  WAITED       3   1  sleep
```

**What to Verify:**
- All three sleep processes run concurrently in background
- Job IDs assigned sequentially [1], [2], [3]
- PIDs assigned in order (3, 4, 5)
- All complete and report "Done"
- Shell reaps each zombie in completion order

---

## Demo 6: All Shell Built-ins

### cat

```bash
$ ./bin/pennos midfs
$ cat d1
```

**Expected Output:**
```
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
abcXYZ\n
a
```

```bash
$ cat d1 d2
```

**Expected Output:**
```
abcXYZ\n
(repeated 14 times)
a

The programs included with the Ubuntu system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Ubuntu comes with ABSOLUTELY NO WARRANTY, to the extent permitted by
appli
```

```bash
$ cat < d1
```

**Expected Output:**
```
abcXYZ\n
(repeated 14 times)
a
```

```bash
$ cat d1 > output
$ cat output
```

**Expected Output:**
```
abcXYZ\n
(repeated 14 times)
a
```

```bash
$ cat d2 >> output
$ cat output
```

**Expected Output:**
```
abcXYZ\n
(repeated 14 times)
a

The programs included with the Ubuntu system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Ubuntu comes with ABSOLUTELY NO WARRANTY, to the extent permitted by
appli
```

---

### echo

```bash
$ echo hello world
```

**Expected Output:**
```
hello world
```

```bash
$ echo test > testfile
$ cat testfile
```

**Expected Output:**
```
test
```

```bash
$ echo more >> testfile
$ cat testfile
```

**Expected Output:**
```
test
more
```

---

### ls

```bash
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> output
  6 -rw-  10 <date> <time> testfile
```

**What to Verify:**
- Shows first block number, permissions, size, timestamp, filename
- All files listed

---

### touch

```bash
$ touch newfile1 newfile2 newfile3
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> output
  6 -rw-  10 <date> <time> testfile
    -rw-   0 <date> <time> newfile1
    -rw-   0 <date> <time> newfile2
    -rw-   0 <date> <time> newfile3
```

**What to Verify:**
- New files created with 0 bytes
- If files exist, only timestamp updated

---

### mv

```bash
$ mv newfile1 renamed1
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> output
  6 -rw-  10 <date> <time> testfile
    -rw-   0 <date> <time> renamed1
    -rw-   0 <date> <time> newfile2
    -rw-   0 <date> <time> newfile3
```

**What to Verify:**
- newfile1 renamed to renamed1
- File content and attributes preserved

---

### cp

```bash
$ cp d1 d1_copy
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  7 -rw- 128 <date> <time> d1_copy
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> output
  6 -rw-  10 <date> <time> testfile
    -rw-   0 <date> <time> renamed1
    -rw-   0 <date> <time> newfile2
    -rw-   0 <date> <time> newfile3
```

```bash
$ cat d1_copy
```

**Expected Output:**
```
abcXYZ\n
(repeated 14 times)
a
```

**What to Verify:**
- d1_copy created with same content as d1
- Both files independent (different blocks)

---

### rm

```bash
$ rm newfile2 newfile3
$ ls
```

**Expected Output:**
```
  2 -rw- 128 <date> <time> d1
  7 -rw- 128 <date> <time> d1_copy
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> output
  6 -rw-  10 <date> <time> testfile
    -rw-   0 <date> <time> renamed1
```

**What to Verify:**
- newfile2 and newfile3 removed
- Blocks freed in FAT
- Other files unaffected

---

### chmod

```bash
$ chmod +x d1
$ chmod +rw d2
$ chmod -wx renamed1
$ ls
```

**Expected Output:**
```
  2 -rwx 128 <date> <time> d1
  7 -rw- 128 <date> <time> d1_copy
  3 -rw- 256 <date> <time> d2
  4 -rw- 128 <date> <time> d3
  5 -rw- 384 <date> <time> output
  6 -rw-  10 <date> <time> testfile
    -r-- 0 <date> <time> renamed1
```

**What to Verify:**
- d1: executable bit added (rw- → rwx)
- d2: already has rw, no change visible
- renamed1: write and execute removed (rw- → r--)
- Permission encoding: 0=none, 2=w, 4=r, 5=rx, 6=rw, 7=rwx

---

### ps

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  8    2   1  R   ps
```

**What to Verify:**
- Shows PID, PPID, priority, status, command name
- init (PID 1), shell (PID 2), ps (current command)
- Status: R=running, B=blocked, S=stopped

---

### kill

```bash
$ sleep 100 &
[1] 9
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  9    2   1  S   sleep
 10    2   1  R   ps
```

```bash
$ kill -term 9
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
 11    2   1  R   ps
```

**What to Verify:**
- sleep terminated by P_SIGTERM signal
- Process 9 no longer appears in ps
- Log shows SIGNALED entry

**Test kill -stop and -cont:**

```bash
$ sleep 100 &
[1] 12
$ kill -stop 12
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
 12    2   1  S   sleep
 13    2   1  R   ps
```

```bash
$ kill -cont 12
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
 12    2   1  S   sleep
 14    2   1  R   ps
```

```bash
$ kill -term 12
```

---

### nice

```bash
$ nice 2 cat d1
```

**Expected Output:**
```
abcXYZ\n
(repeated 14 times)
a
```

**Verify in Log:**
```bash
$ logout
$ cat log | grep "CREATE.*cat"
```

**Expected:**
```
[ XX]  CREATE      15   2  cat
```

**What to Verify:**
- cat runs with priority 2 (not default 1)
- Command executes correctly with custom priority

---

### nice_pid

```bash
$ ./bin/pennos midfs
$ sleep 100 &
[1] 3
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   1  S   sleep
  4    2   1  R   ps
```

```bash
$ nice_pid 0 3
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  3    2   0  S   sleep
  5    2   1  R   ps
```

**Verify in Log:**
```bash
$ logout
$ cat log | grep "NICE.*3"
```

**Expected:**
```
[ XX]  NICE         3   1  0  sleep
```

**What to Verify:**
- Priority changed from 1 to 0 for PID 3
- Log shows NICE event with old and new priorities

---

### man

```bash
$ ./bin/pennos midfs
$ man
```

**Expected Output:**
```
Available commands:
  cat          - Concatenate files and print to stdout
  sleep        - Sleep for specified ticks
  busy         - Busy wait indefinitely
  echo         - Echo arguments to stdout
  ls           - List directory contents
  touch        - Create file or update timestamp
  mv           - Move/rename file
  cp           - Copy file
  rm           - Remove file(s)
  chmod        - Change file permissions
  ps           - List processes
  kill         - Send signal to process
  zombify      - Create zombie process (testing)
  orphanify    - Create orphan process (testing)
  nice         - Run command with priority
  nice_pid     - Change priority of existing process
  man          - Display this help
  bg           - Resume job in background
  fg           - Bring job to foreground
  jobs         - List jobs
  logout       - Exit shell
```

**What to Verify:**
- All implemented commands listed
- Brief description for each

---

### jobs

```bash
$ sleep 10 &
[1] 6
$ sleep 20 &
[2] 7
$ jobs
```

**Expected Output:**
```
[1]- Running                 sleep 10 &
[2]+ Running                 sleep 20 &
```

```bash
$ kill -term 6 7
```

---

### logout

```bash
$ logout
```

**Expected Output:**
```
(PennOS exits, returns to shell)
```

**What to Verify:**
- Shell process exits cleanly
- PennOS shuts down
- All processes terminated
- Log file written

---

## Demo 7: Shell Scripts

```bash
$ ./bin/pennos midfs
$ echo echo line1 > script
$ echo echo line2 >> script
$ cat script
```

**Expected Output:**
```
echo line1
echo line2
```

```bash
$ chmod +x script
$ ls | grep script
```

**Expected Output:**
```
    -rwx   20 <date> <time> script
```

```bash
$ script > out
$ cat out
```

**Expected Output:**
```
line1
line2
```

```bash
$ logout
```

**What to Verify:**
- Script file created with commands
- chmod adds executable permission
- Script executes line by line
- Output redirected to file
- Each echo command runs as separate process

---

## Demo 8: Crash Test

**Prerequisite:** Create a filesystem with sufficient space (at least 5480 bytes free)

```bash
$ ./bin/pennos <fatfs_with_space>
$ crash
```

**Expected Behavior:**
- Program should handle rapid file operations
- Create files, write data, delete files
- May stress test scheduler, file system
- Should not crash the OS
- Should complete or be interruptible with Ctrl-C

```bash
$ ps
```

**Expected Output:**
```
PID PPID PRI STAT CMD
  1    0   0  B   init
  2    1   0  B   shell
  X    2   1  R   ps
```

```bash
$ logout
```

**What to Verify:**
- OS remains stable under stress
- File system operations complete correctly
- No memory leaks or corruption
- Proper process cleanup

---

# VERIFICATION CHECKLIST

## Kernel Verification
- [ ] Zombie processes properly created and reaped
- [ ] Orphan processes adopted by init
- [ ] Blocking and non-blocking wait work correctly
- [ ] Priority scheduling ratios correct (1.5x between levels)
- [ ] No process starvation
- [ ] Idling works when all processes blocked (0% CPU)
- [ ] Log file format correct and complete
- [ ] Signal handling (stop, cont, term) works

## File System Verification
- [ ] FAT formatted correctly (block size, FAT size)
- [ ] Files created, read, written, deleted correctly
- [ ] Block allocation and deallocation works
- [ ] Directory expansion works (multi-block directories)
- [ ] File operations update timestamps
- [ ] Deleted files marked correctly (0x01 in name[0])
- [ ] Non-contiguous allocation works
- [ ] Mount/unmount preserves data

## Shell Verification
- [ ] All built-ins work correctly
- [ ] I/O redirection works (>, <, >>)
- [ ] Job control works (fg, bg, jobs)
- [ ] Ctrl-Z stops foreground process
- [ ] Ctrl-C terminates foreground process
- [ ] Background jobs run concurrently
- [ ] Shell scripts execute correctly
- [ ] Permissions checked and enforced

## Integration Verification
- [ ] File operations use s_ system calls
- [ ] System calls use k_ kernel functions
- [ ] Proper abstraction maintained
- [ ] Error handling works throughout
- [ ] No memory leaks (valgrind clean if possible)

---

# TESTING TIPS

1. **Always check logs** after kernel demos
2. **Use hd command** to verify file system structure
3. **Monitor CPU** with `top` during scheduling demos
4. **Test error cases** (file not found, permission denied)
5. **Verify proper cleanup** after each test
6. **Check process counts** with ps frequently
7. **Test edge cases** (empty files, max size files, etc.)
8. **Verify timestamps** update correctly
9. **Test concurrent operations** (multiple background jobs)
10. **Document any deviations** from expected output

---

**END OF DEMO GUIDE**