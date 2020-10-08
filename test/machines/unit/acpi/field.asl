DefinitionBlock ("", "DSDT", 1, "UMKA ", "UMKADSDT", 0x00000001)
{
    Name (BUFZ, Buffer (32) {0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99})
    CreateField (BUFZ,  0, 2, FBZ0)

    Name (B000, Buffer (17) {0x06, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
    CreateField (B000,  0, 1, FB00)
    CreateField (B000,  1, 1, FB01)
    CreateField (B000,  2, 1, FB02)
    CreateField (B000,  3, 1, FB03)
    CreateField (B000,  4, 1, FB04)
    CreateField (B000,  5, 1, FB05)
    CreateField (B000,  6, 1, FB06)
    CreateField (B000,  7, 1, FB07)

    CreateField (B000,  8, 1, FB08)
    CreateField (B000,  9, 1, FB09)
    CreateField (B000, 10, 1, FB10)
    CreateField (B000, 11, 1, FB11)
    CreateField (B000, 12, 1, FB12)
    CreateField (B000, 13, 1, FB13)
    CreateField (B000, 14, 1, FB14)
    CreateField (B000, 15, 1, FB15)

    Name (INT5, 5)
    Name (INTX, 0)

    Method (PARG, 1) {
        Store (Arg0, INTX)
        printf("### %o\n", INTX)
    }

    Method (MAIN, 0)
    {
/*
        Debug = FB00
        Debug = FB01
        Debug = FB02
        Debug = FB03
        Debug = FB04
        Debug = FB05
        Debug = FB06
        Debug = FB07
        Debug = FB08
        Debug = FB09
        Debug = FB10
        Debug = FB11
        Debug = FB12
        Debug = FB13
        Debug = FB14
        Debug = FB15

        Debug = BUFZ
        Debug = FBZ0
        BUFZ = B000
        Debug = BUFZ
        Debug = FBZ0
        FBZ0 = 0
        Debug = BUFZ
        Debug = FBZ0
        Debug = B000
*/
        PARG(5)
        PARG(INT5)
    }
}
