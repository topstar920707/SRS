/*
# see: https://github.com/winlinvip/simple-rtmp-server/issues/190
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/v1_CN_SrsLinuxArm
    g++ -g -O0 -o jmp_sp jmp_sp.cpp
    arm-linux-gnueabi-g++ -g -o jmp_sp jmp_sp.cpp -static
    arm-linux-gnueabi-strip jmp_sp
*/
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

jmp_buf env_func1;

void func1()
{
#if defined(__amd64__) || defined(__x86_64__)
    register long int rsp0 asm("rsp");
    
    int ret = setjmp(env_func1);
    printf("setjmp func0 ret=%d, rsp=%#lx\n", ret, rsp0);
#endif
}

void func0()
{
    /**
    the definition of jmp_buf:
        typedef struct __jmp_buf_tag jmp_buf[1];
        struct __jmp_buf_tag {
             __jmp_buf __jmpbuf;
             int __mask_was_saved;
             __sigset_t __saved_mask;
        };
    */
#if defined(__amd64__) || defined(__x86_64__)
    /**
    here, the __jmp_buf is 8*8=64 bytes
        # if __WORDSIZE == 64
            typedef long int __jmp_buf[8];
    */
    /**
    the layout for setjmp of x86_64
        #
        # The jmp_buf is assumed to contain the following, in order:
        #       %rbx
        #       %rsp (post-return)
        #       %rbp
        #       %r12
        #       %r13
        #       %r14
        #       %r15
        #       <return address>
        #
    */
    register long int rsp0 asm("rsp");
    
    int ret = setjmp(env_func1);
    printf("setjmp func0 ret=%d, rsp=%#lx\n", ret, rsp0);
    
    printf("after setjmp: ");
    for (int i = 0; i < 8; i++) {
        printf("env[%d]=%#x, ", i, (int)env_func1[0].__jmpbuf[i]);
    }
    printf("\n");
    
    func1();
#else
    /**
        /usr/arm-linux-gnueabi/include/bits/setjmp.h
        #ifndef _ASM
        The exact set of registers saved may depend on the particular core
           in use, as some coprocessor registers may need to be saved.  The C
           Library ABI requires that the buffer be 8-byte aligned, and
           recommends that the buffer contain 64 words.  The first 28 words
           are occupied by v1-v6, sl, fp, sp, pc, d8-d15, and fpscr.  (Note
           that d8-15 require 17 words, due to the use of fstmx.)
        typedef int __jmp_buf[64] __attribute__((__aligned__ (8)));
        
        the layout of setjmp for arm:
            0-5: v1-v6 
            6: sl
            7: fp
            8: sp
            9: pc
            10-26: d8-d15 17words
            27: fpscr
    */
    /**
    For example, on raspberry-pi, armv6 cpu:
        (gdb) x /64 env_func1[0].__jmpbuf
            v1, 0:  0x00	0x00	0x00	0x00	
            v2, 1:  0x00	0x00	0x00	0x00
            v3, 2:  0x2c	0x84	0x00	0x00	
            v4, 3:  0x00	0x00	0x00	0x00
            v5, 4:  0x00	0x00	0x00	0x00	
            v6, 5:  0x00	0x00	0x00	0x00
            sl, 6:  0x00	0xf0	0xff	0xb6	
            fp, 7:  0x9c	0xfb	0xff	0xbe
            sp, 8:  0x88	0xfb	0xff	0xbe	
            pc, 9:  0x08	0x85	0x00	0x00
        (gdb) p /x $sp
        $5 = 0xbefffb88
        (gdb) p /x $pc
        $4 = 0x850c
    */
    int ret = setjmp(env_func1);
    printf("setjmp func1 ret=%d\n", ret);
    
    printf("after setjmp: ");
    for (int i = 0; i < 64; i++) {
        printf("env[%d]=%#x, ", i, (int)env_func1[0].__jmpbuf[i]);
    }
    
    printf("func0 terminated\n");
#endif
}

int main(int argc, char** argv) {
#if defined(__amd64__) || defined(__x86_64__)
    printf("x86_64 sizeof(long int)=%d, sizeof(long)=%d, sizeof(int)=%d, __WORDSIZE=%d\n", 
        (int)sizeof(long int), (int)sizeof(long), (int)sizeof(int), (int)__WORDSIZE);
#else
    printf("arm sizeof(long int)=%d, sizeof(long)=%d, sizeof(int)=%d\n", 
        (int)sizeof(long int), (int)sizeof(long), (int)sizeof(int));
#endif

    func0();
    
    printf("terminated\n");

    return 0;
}
