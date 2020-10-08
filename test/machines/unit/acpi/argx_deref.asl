DefinitionBlock ("", "DSDT", 1, "BLAH", "BLAHDSDT", 0x00000001)
{
    Name (INT5, 5)
    Name (INTX, 0)

    Method (DRFA, 1) {
        printf("Arg0: %o", DeRefOf(Arg0))
        printf("Arg0: %o", Arg0)
        Store (DeRefOf(Arg0), INTX)
        Store (Arg0, INTX)
        Debug = INTX
    }

    Method (MAIN, 0)
    {
        DRFA(RefOf(INT5))
    }
}
