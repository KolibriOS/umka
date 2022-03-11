KOLIBRIOS = tup.getconfig("KOLIBRIOS")
EXE_SUFFIX = ""

WARNINGS = " -Wall -Wextra -Wnull-dereference -Wshadow -Wformat=2 -Wswitch -Wswitch-enum  -Wpedantic"
--  -Wconversion -Wsign-conversion
NOWARNINGS = " -Wno-address-of-packed-member"

CC = tup.getconfig("CC")
if string.find(CC, "gcc") then
    WARNINGS = WARNINGS .. " -Wduplicated-cond -Wduplicated-branches -Wrestrict -Wlogical-op -Wjump-misses-init"
elseif string.find(CC, "clang") then
    NOWARNINGS = NOWARNINGS .. " -Wno-missing-prototype-for-cc"
else
    print("[!] Your compiler is not supported")
end

CFLAGS = WARNINGS .. NOWARNINGS .. " -std=c11 -g -O0 -D_FILE_OFFSET_BITS=64 -DNDEBUG -masm=intel -D_POSIX_C_SOURCE=200809L -I$(HOST) -fno-pie"
CFLAGS_32 = CFLAGS .. " -m32"
LDFLAGS = " -no-pie"
LDFLAGS_32 = LDFLAGS .. " -m32"


HOST = tup.getconfig("TUP_PLATFORM")
if HOST == "win32" then
    HOST = "windows"
    EXE_SUFFIX = ".exe"
elseif HOST == "linux" then
    HOST = "linux"
    EXE_SUFFIX = ""
else
    print("[!] Platform " .. HOST .. " is not supported")
end

FASM_INCLUDE = KOLIBRIOS .. "/kernel/trunk;" .. KOLIBRIOS .. "/programs/develop/libraries/libcrash/hash"
FASM_FLAGS = " -dUEFI=1 -dextended_primary_loader=1 -dUMKA=1 -dHOST=" .. HOST .. " -m 2000000"

if HOST == "linux" then
--        FASM_INCLUDE=$(KOLIBRIOS)/kernel/trunk;$(KOLIBRIOS)/programs/develop/libraries/libcrash/hash
    FASM = 'INCLUDE="$(FASM_INCLUDE)" fasm ' .. FASM_FLAGS
elseif HOST == "windows" then
--  FASM_INCLUDE=$(KOLIBRIOS)\kernel\trunk;$(KOLIBRIOS)\programs\develop\libraries\libcrash\hash
    FASM = 'set INCLUDE=' .. FASM_INCLUDE .. '&&fasm ' .. FASM_FLAGS
else
    print("[!] Your OS is not supported")
end



tup.rule("umka.asm", FASM .. " %f umka.o -s umka.fas", {"umka.o", "umka.fas"})

tup.rule(HOST .. "/pci.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule(HOST .. "/thread.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("getopt.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("shell.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("util.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("vdisk.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("vnet.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("lodepng.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("trace.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("trace_lbr.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")

tup.rule("umka_shell.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("umka_fuse.c", CC .. CFLAGS_32 .. " `pkg-config fuse3 --cflags` -c %f", "%B.o")
tup.rule("umka_os.c", CC .. CFLAGS_32 .. " -c %f", "%B.o")
tup.rule("umka_ping.c", CC .. CFLAGS_32 .. " -D_DEFAULT_SOURCE -c %f", "%B.o")

tup.rule({"umka_shell.o", "shell.o", "pci.o", "thread.o", "vnet.o", "vdisk.o",
          "trace.o", "trace_lbr.o", "umka.o", "lodepng.o", "util.o",
          extra_inputs = {"umka.ld"}
         },
         CC .. LDFLAGS_32 .. " %f -o %o -T umka.ld",
         "umka_shell" .. EXE_SUFFIX)

tup.rule({"umka_fuse.o", "shell.o", "pci.o", "thread.o", "vnet.o", "vdisk.o",
          "trace.o", "trace_lbr.o", "umka.o", "lodepng.o", "util.o"},
         CC .. LDFLAGS_32 .. " %f -o %o `pkg-config fuse3 --libs`",
         "umka_fuse" .. EXE_SUFFIX)

tup.rule({"umka_os.o", "umka_ping.o", "shell.o", "pci.o", "thread.o", "vnet.o",
          "vdisk.o", "trace.o", "trace_lbr.o", "umka.o", "lodepng.o", "util.o"},
         CC .. LDFLAGS_32 .. " %f -o %o",
         "umka_os" .. EXE_SUFFIX)
