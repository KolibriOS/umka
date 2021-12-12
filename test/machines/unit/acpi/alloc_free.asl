DefinitionBlock ("", "DSDT", 1, "UMKA ", "UMKADSDT", 0x00000001)
{

    Name (INT5, 5)
    Name (REF5, 2)

    Method (AREF, 1) {
        Debug = INT5
        Debug = Arg0
        Debug = REF5

        Store("\n", Debug)

        Local0 = RefOf(Arg0)
        CopyObject(Local0, REF5)

        CopyObject(RefOf(Arg0), REF5)
        Local1 = DerefOf(Local0)
        Local2 = DerefOf(Local0)
        Local3 = RefOf(Local0)
        Local4 = RefOf(REF5)
        Local5 = RefOf(Local3)
        Local6 = RefOf(Local5)
        Local7 = RefOf(Local6)

        Debug = Local0
        Debug = Local1
        Debug = Local2
        Debug = Local3
        Debug = Local4
        Debug = Local5
        Debug = Local6
        Debug = Local7
        Debug = INT5
        Debug = REF5
        Debug = DeRefOf(REF5)
        Debug = DerefOf(Local0)

        DeRefOf(Local0) = 0
        DeRefOf(Local3) = 3
        DeRefOf(Local4) = 4
        DeRefOf(Local7) = 7
//        DeRefOf(REF5) = 3

        Local0 = 3
        Local1 = 3
        Local2 = 3

        Store("\n", Debug)
        Debug = Local0
        Debug = Local1
        Debug = Local2
        Debug = Local3
        Debug = Local4
        Debug = Local5
        Debug = Local6
        Debug = Local7
        Debug = INT5
        Debug = Arg0
        Debug = REF5
        Debug = DeRefOf(REF5)
    }

    Method (MAIN, 0) {
        AREF(INT5)
    }
}
