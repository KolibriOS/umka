DefinitionBlock ("", "DSDT", 1, "UMKA ", "UMKADSDT", 0x00000001)
{
    Name (BUFF, Buffer (4) {0x66, 0x72, 0x6F, 0x6D})

    Method (TEST, 0, NotSerialized)
    {
        Debug = "hello"
        Debug = 0x42
        printf("hello")
        printf("hi %x there", 0x42)
        printf("hello %o buffer", BUFF)
    }
}
