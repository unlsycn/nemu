#include <isa.h>
#include <memory/paddr.h>

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
static const uint32_t img[] = {
    0x3e800093, 0xf3938113, 0x0020c863, 0x0186af37, 0x001f0f1b, 0x00100073, 0x0186afb7, 0x002f8f9b, 0x00100073,
};

static void restart()
{
    /* Set the initial program counter. */
    cpu.pc = RESET_VECTOR;

    /* The zero register is always 0. */
    cpu.gpr[0] = 0;
}

void init_isa()
{
    /* Load built-in image. */
    memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));

    /* Initialize this virtual computer system. */
    restart();
}
