#include<stdlib.h>
#include<iostream>
#include<iterator>
#include<array>
#include<map>
#include "../WS2812B_Controller.h"

int main() {
    int n2 = 6;
    int lo = 9;
    int n1 = 2;
    asm (
        "mov  %1, %0 \n\t" 
        "add  %2, %1" 
          : "=r" (n2) , "+ra" (n1)
          : "r" (lo), "a" (n2)
          :
    );
    std::cout<<n1;
}