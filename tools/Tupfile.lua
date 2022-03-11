CC = tup.getconfig("CC")

EXE_SUFFIX = ""
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

WARNINGS = " -Wall -Wextra -Wduplicated-cond -Wduplicated-branches -Wlogical-op"
         .. " -Wrestrict -Wnull-dereference -Wjump-misses-init -Wshadow"
         .. " -Wformat=2 -Wswitch -Wswitch-enum -Wpedantic"
NOWARNINGS = " -Wno-address-of-packed-member"
CFLAGS = WARNINGS .. NOWARNINGS .. " -std=c11 -O2"
       .. " -DNDEBUG -D_POSIX_C_SOURCE=200809L -fno-pie"
LDFLAGS = " -no-pie"

tup.rule("mkdirrange.c", CC .. CFLAGS .. LDFLAGS .. " %f -o %o", "%B" .. EXE_SUFFIX)
tup.rule("mkfilepattern.c", CC .. CFLAGS .. LDFLAGS .. " %f -o %o", "%B" .. EXE_SUFFIX)
tup.rule("lfbviewx.c", CC .. CFLAGS .. LDFLAGS .. " %f -o %o -lX11 -lXext -D_GNU_SOURCE", "%B" .. EXE_SUFFIX)
tup.rule("randdir.c", CC .. CFLAGS .. LDFLAGS .. " %f -o %o", "%B" .. EXE_SUFFIX)
