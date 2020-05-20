DefinitionBlock ("", "DSDT", 2, "UMKA ", "UMKADSDT", 0x00000000)
{
    Device (PCI0) {
        Name (_ADR, Zero)
    }

    Scope (\PCI0) {
        Device (DV00) {
            Name (_ADR, One)
            Name (DV00, Zero)
        }
        Name (X, 3)
        Scope (\) {
            Method (RQ) {
                Return (0)
            }
        }
        Name (^Y, 4)
    }

    Alias (PCI0, ALA0)
    Alias (PCI0.DV00, AL01)
    Alias (PCI0.DV00.DV00, AL02)
    Alias (\PCI0, AL03)
    Alias (\PCI0.DV00, AL04)
    Alias (\PCI0.DV00.DV00, AL05)

    Scope (\) {
        Alias (PCI0, AL06)
        Alias (PCI0.DV00, AL07)
        Alias (PCI0.DV00.DV00, AL08)
        Alias (\PCI0, AL09)
        Alias (\PCI0.DV00, AL10)
        Alias (\PCI0.DV00.DV00, AL11)
    }

    Scope (PCI0) {
        Alias (DV00, AL00)
        Alias (DV00.DV00, AL01)
        Alias (\PCI0, AL02)
        Alias (\PCI0.DV00, AL03)
        Alias (\PCI0.DV00.DV00, AL04)
        Alias (^PCI0, AL05)
        Alias (^PCI0.DV00, AL06)
        Alias (^PCI0.DV00.DV00, AL07)

        Scope (^PCI0.DV00) {
            Alias (DV00, AL00)
            Alias (\PCI0, AL01)
            Alias (\PCI0.DV00, AL02)
            Alias (\PCI0.DV00.DV00, AL03)
            Alias (^DV00, AL04)
            Alias (^DV00.DV00, AL05)
            Alias (^^PCI0, AL06)
            Alias (^^PCI0.DV00, AL07)
            Alias (^^PCI0.DV00.DV00, AL08)
        }
    }

    Scope (PCI0.DV00) {
            Alias (DV00, AL09)
            Alias (\PCI0, AL10)
            Alias (\PCI0.DV00, AL11)
            Alias (\PCI0.DV00.DV00, AL12)
            Alias (^DV00, AL13)
            Alias (^DV00.DV00, AL14)
            Alias (^^PCI0, AL15)
            Alias (^^PCI0.DV00, AL16)
            Alias (^^PCI0.DV00.DV00, AL17)
    }

    Scope (\) {
        Device (DV0A) {Name (_ADR, 0)}
        Device (PCI0.DV0A) {Name (_ADR, 0)}
        Device (PCI0.DV00.DV0A) {Name (_ADR, 0)}
        Device (\DV0B) {Name (_ADR, 0)}
        Device (\PCI0.DV0B) {Name (_ADR, 0)}
        Device (\PCI0.DV00.DV0B) {Name (_ADR, 0)}
    }

    Scope (PCI0) {
        Device (DV0C) {Name (_ADR, 0)}
        Device (DV00.DV0C) {Name (_ADR, 0)}
        Device (\DV0D) {Name (_ADR, 0)}
        Device (\PCI0.DV0D) {Name (_ADR, 0)}
        Device (\PCI0.DV00.DV0D) {Name (_ADR, 0)}
        Device (^DV0E) {Name (_ADR, 0)}
        Device (^PCI0.DV0E) {Name (_ADR, 0)}
        Device (^PCI0.DV00.DV0E) {Name (_ADR, 0)}

        Scope (^PCI0.DV00) {
            Device (DV0F) {Name (_ADR, 0)}
            Device (\DV0F) {Name (_ADR, 0)}
            Device (\PCI0.DV0F) {Name (_ADR, 0)}
            Device (\PCI0.DV00.DV0G) {Name (_ADR, 0)}
            Device (^DV0G) {Name (_ADR, 0)}
            Device (^DV00.DV0H) {Name (_ADR, 0)}
            Device (^^DV0H) {Name (_ADR, 0)}
            Device (^^PCI0.DV0H) {Name (_ADR, 0)}
            Device (^^PCI0.DV00.DV0I) {Name (_ADR, 0)}
        }
    }

    Name (PKG0, Package () {
        DV0A,
        PCI0.DV0A,
        PCI0.DV00.DV0A,
        \DV0A,
        \PCI0.DV0A,
        \PCI0.DV00.DV0A,

        Package () {
            DV0A,
            PCI0.DV0A,
            PCI0.DV00.DV0A,
            \DV0A,
            \PCI0.DV0A,
            \PCI0.DV00.DV0A,

            Package () {
                DV0A,
                PCI0.DV0A,
                PCI0.DV00.DV0A,
                \DV0A,
                \PCI0.DV0A,
                \PCI0.DV00.DV0A,
            }
        }
    })

    Scope (\PCI0) {
    Name (PKG1, Package () {
        DV0A,
        DV00.DV0A,
        \DV0A,
        \PCI0.DV0A,
        \PCI0.DV00.DV0A,
        ^DV0A,
        ^PCI0.DV0A,
        ^PCI0.DV00.DV0A,

        Package () {
            DV0A,
            DV00.DV0A,
            \DV0A,
            \PCI0.DV0A,
            \PCI0.DV00.DV0A,
            ^DV0A,
            ^PCI0.DV0A,
            ^PCI0.DV00.DV0A,

            Package () {
                DV0A,
                DV00.DV0A,
                \DV0A,
                \PCI0.DV0A,
                \PCI0.DV00.DV0A,
                ^DV0A,
                ^PCI0.DV0A,
                ^PCI0.DV00.DV0A,
            }
        }
    })

        Method (M000, 0, NotSerialized)
        {
        }

        Method (M001, 0, NotSerialized)
        {
            Return (\DV0A)
        }

        Method (M002, 0, NotSerialized)
        {
            Return (DV0A)
        }

        Method (M003, 0, NotSerialized)
        {
            Return (^DV0A)
        }

        Method (M004, 0, NotSerialized)
        {
            Return (\PCI0.DV0A)
        }

        Method (M005, 0, NotSerialized)
        {
            Return (^^PKG0)
        }

        Method (M006, 0, NotSerialized)
        {
            Return (^^PCI0.DV0A)
        }
    }
}
