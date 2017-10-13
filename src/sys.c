//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

#include <stdint.h>
#include <dma.h>
#include <n64sys.h>
#include "sys.h"
#include "rom.h"


u32 asm_date;

Options_st options;

u32 native_tv_mode;

void dma_read_s(void * ram_address, unsigned long pi_address, unsigned long len) {

    u32 buff[256];
   
    u32 *bptr;
    
      u32 *rptr = (u32 *) ram_address;
    
   // if(len==32768)
  //  rptr = (u32 *) 0x803F7988;

    u16 i;

    //u16 blen = 512;
    //if (len < 512)blen = len;
    //*(volatile u32*) 0x1FC007FC = 0x08;

    IO_WRITE(PI_STATUS_REG, 3);
    while (len) {
        dma_read(buff, pi_address, 512);
        while ((IO_READ(PI_STATUS_REG) & 3) != 0);
        //while ((*((volatile u32*) PI_STATUS_REG) & 0x02) != 1);
        data_cache_hit_invalidate(buff, 512);
        bptr = buff;
        for (i = 0; i < 512 && i < len; i += 4)*rptr++ = *bptr++;
        len = len < 512 ? 0 : len - 512;
        pi_address += 512;
    }
}

void dma_write_s(void * ram_address, unsigned long pi_address, unsigned long len) {
	
	
	//if(len==32768)
	//ram_address = (u32 *) 0x803F7988;


    data_cache_hit_writeback(ram_address, len);
    dma_write(ram_address, pi_address, len);

		
    
}


/*
void showError(char *str, u32 code) {


    console_printf("%s%u\n", str, code);
    joyWait();


}
 */
void sleep(u32 ms) {

    u32 current_ms = get_ticks_ms();

    while (get_ticks_ms() - current_ms < ms);

}

void dma_read_sram(void *dest, u32 offset, u32 size) {
    /*
        PI_DMAWait();

        IO_WRITE(PI_STATUS_REG, 0x03);
        IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
        IO_WRITE(PI_CART_ADDR_REG, (0xA8000000 + offset));
       // data_cache_invalidate_all();
        IO_WRITE(PI_WR_LEN_REG, (size - 1));
        */
     /* 0xA8000000
     *  0xb0000000
     *  0x4000000
     * */
    dma_read_s(dest, 0xA8000000 + offset, size);
    //data_cache_invalidate(dest,size);

}

void dma_write_sram(void* src, u32 offset, u32 size) {
    /*
        PI_DMAWait();

        IO_WRITE(PI_STATUS_REG, 0x02);
        IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
        IO_WRITE(PI_CART_ADDR_REG, (0xA8000000 + offset));
      //  data_cache_invalidate_all();
        IO_WRITE(PI_RD_LEN_REG, (size - 1));
	*/
    dma_write_s(src, 0xA8000000 + offset, size);

}


//TODO: look at https://github.com/mamedev/mame/blob/master/3rdparty/zlib/crc32.c
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

unsigned int CRC_Calculate(unsigned int crc, unsigned char* buf, unsigned int len) {
    static unsigned int crc_table[256];
    static int make_crc_table = 1;

    if (make_crc_table) {
        unsigned int c, n;
        int k;
        unsigned int poly;
        const unsigned char p[] = {0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26};

        /* make exclusive-or pattern from polynomial (0xedb88320L) */
        poly = 0L;
        for (n = 0; n < sizeof (p) / sizeof (unsigned char); n++)
            poly |= 1L << (31 - p[n]);

        for (n = 0; n < 256; n++) {
            c = n;
            for (k = 0; k < 8; k++)
                c = c & 1 ? poly ^ (c >> 1) : c >> 1;
            crc_table[n] = c;
        }
        make_crc_table = 0;
    }

    if (buf == (void*) 0) return 0L;

    crc = crc ^ 0xffffffffL;
    while (len >= 8) {
        DO8(buf);
        len -= 8;
    }
    if (len)
        do {
            DO1(buf);
        } while (--len);

    return crc ^ 0xffffffffL;
}



u32 ii;
volatile u32 *pt;
void clean();

#define MEM32(addr) *((volatile u32 *)addr)


u8 STR_intToDecString(u32 val, u8 *str) {

    int len;

    if (val < 10)len = 1;
    else
        if (val < 100)len = 2;
    else
        if (val < 1000)len = 3;
    else
        if (val < 10000)len = 4;
    else
        if (val < 100000)len = 5;
    else
        if (val < 1000000)len = 6;
    else
        if (val < 10000000)len = 7;
    else
        if (val < 100000000)len = 8;
    else
        if (val < 1000000000)len = 9;
    else len = 10;

    str += len;
    str[0] = 0;
    if (val == 0)*--str = '0';
    while (val) {

        *--str = '0' + val % 10;
        val /= 10;
    }


    return len;
}

void STR_intToDecStringMin(u32 val, u8 *str, u8 min_size) {

    int len;
    u8 i;

    if (val < 10)len = 1;
    else
        if (val < 100)len = 2;
    else
        if (val < 1000)len = 3;
    else
        if (val < 10000)len = 4;
    else
        if (val < 100000)len = 5;
    else
        if (val < 1000000)len = 6;
    else
        if (val < 10000000)len = 7;
    else
        if (val < 100000000)len = 8;
    else
        if (val < 1000000000)len = 9;
    else len = 10;

    if (len < min_size) {

        i = min_size - len;
        while (i--)str[i] = '0';
        len = min_size;
    }
    str += len;
    str[0] = 0;
    if (val == 0)*--str = '0';
    while (val) {

        *--str = '0' + val % 10;
        val /= 10;
    }
}



