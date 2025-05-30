#include <iostream>
#include <stdio.h>

int main() {
  printf("\033c");                        // clear screen
  printf("\033[3;1H");                    // move to 3rd line
  printf("Merry Christmas");              // print text
  printf("\033[5;30H");                   // move to 5th line
  printf("Merry Christmas to you too ");  // print text

  return 0;
}
