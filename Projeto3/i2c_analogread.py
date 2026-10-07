from smbus import SMBus

addr = 0x8
bus = SMBus(1)

val8 = 0
val10 = 0

numb = 1

while numb:
    val8 = bus.read_byte(addr)
    val10 = val8 * 4

    print(val10)


