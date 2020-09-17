DefinitionBlock ("", "DSDT", 1, "UMKA ", "UMKADSDT", 0x00000001)
{
    OperationRegion (M000, SystemMemory, 0xE001, 17)
    Field (M000, ByteAcc, NoLock, Preserve)
    {
        FM00,   1, 
        FM01,   2
    }


    Method (TEST, 0, NotSerialized)
    {
        printf("hello")
    }
}
