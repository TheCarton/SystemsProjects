#include "stdio.h"
int main() {
    printf("it runs\n");
    int *ptr2 = (int *) 0x00200ffc;
    printf("0x%x\n", *ptr2); // printing as hex numbers
}