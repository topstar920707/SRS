/*
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/SrsLinuxArm
 arm-linux-gnueabi-g++ -o test test.cpp -static
 arm-linux-gnueabi-strip test
*/
#include <stdio.h>

int main(int argc, char** argv) {
	printf("hello, arm!\n");
	return 0;
}
