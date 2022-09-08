#include <stdio.h>
#include <stdlib.h>

int HEX7_6_display(int value){
    char display_values[10] = {0x3F, 0x6, 0x5B, 0x4F, 0x66, 0x6D, 0x7C, 0x7, 0x7F, 0x6F}; // Hex value to display 1-9
    int d;
    int d2; // 10's digit
    int d1; // 1's digit

    d1 = display_values[value%10];
    value /= 10;
    d2 = display_values[value%10];
    value /= 10;
    d = (d2 << 24) + (d1 << 16); // Display the value in hex
    return d;
}




int HEX5_4_display(int value){
    char display_values[10] = {0x3F, 0x6, 0x5B, 0x4F, 0x66, 0x6D, 0x7C, 0x7, 0x7F, 0x6F}; // Hex value to display 1-9
    int d;
    int d2; // 10's digit
    int d1; // 1's digit

    d1 = display_values[value%10];
    value /= 10;
    d2 = display_values[value%10];
    d = (d2 << 8) + d1; // Display the value in hex
    return d;
}


int HEX3_0_display(int value){
    char display_values[10] = {0x3F, 0x6, 0x5B, 0x4F, 0x66, 0x6D, 0x7C, 0x7, 0x7F, 0x6F}; // Hex value to display 1-9
    int display;
    int d4; // 10's digit of minute
    int d3; // 1's digit of minute
    int d2; // 10's digit of seconds
    int d1; // 1's digit of seconds

    int minute = value / 60;
    int seconds = value % 60;

    d1 = display_values[seconds%10];
    seconds /= 10;
    d2 = display_values[seconds%10];
    seconds /= 10;
    d3 = display_values[minute%10];
    minute /= 10;
    d4 = display_values[minute%10];
    display =(d4 << 24) + (d3 << 16) + (d2 << 8) + d1; // Display the value in hex
    return display;
}


int HEX2_0_display(int value){
    char display_values[10] = {0x3F, 0x6, 0x5B, 0x4F, 0x66, 0x6D, 0x7C, 0x7, 0x7F, 0x6F}; // Hex value to display 1-9
    int d;
    int d3; // 100's digit of random value
    int d2; // 10's digit of random value
    int d1; // 1's digit of random value

    d1 = display_values[value%10];
    value /= 10;
    d2 = display_values[value%10];
    value /= 10;
    d3 = display_values[value%10];
    d = (d3 << 16) + (d2 << 8) + d1; // Display the value in hex
    return d;
}


int random_value(int upper, int lower){
    int num = (rand() % (upper - lower +1)) + lower;
    return num;
}


int bit_check(int value, int bit){
    if ( ((value >> bit) & 1) ){
        return 1;
    }
    else{
        return 0;
    }
}



