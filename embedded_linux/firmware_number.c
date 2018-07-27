#include <common.h>
#include <command.h>
#include <fs.h>
#include <fat.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>

static int firmware(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    // static int number0 __attribute__ ((__aligned__(16)));
    // char number __attribute__ ((__aligned__(L1_CACHE_BYTES)));
    __u8 number __aligned(ARCH_DMA_MINALIGN);
    char * argv_load[5];
    argv_load[1] = "mmc";
    argv_load[2] = "0:0";
    argv_load[4] = "firmware_selected";

    char number_addr_str[32];
    snprintf(number_addr_str, 32, "%p" , &number);
    argv_load[3] = number_addr_str;
    do_load(cmdtp, flag, 5, argv_load, FS_TYPE_FAT);
    putc(number);
    puts("\n");

    return 1;
}

U_BOOT_CMD_COMPLETE(firmware, 5, 1, firmware, "print firmware version", "arg1: start address", 0);
