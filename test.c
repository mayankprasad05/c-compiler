// gcc all component names -o compiler    ← compile the compiler
// ./compiler test.c             ← run test program

#include <stdio.h>

int main() {
    int a = 10;
    int b = 3;

    printf("a = "); printf(a); printf("\n"); printf("b = "); printf(b); printf("\n");

    int sum = a + b; printf("a + b = "); printf(sum); printf("\n");

    int diff = a - b; printf("a - b = "); printf(diff); printf("\n");

    int prod = a * b; printf("a * b = "); printf(prod); printf("\n");

    int x = 7;
    if (x > 5) {printf("x is greater than 5\n"); } 
    else {
        printf("x is small\n");
    }

    int i = 1;
    int total = 0;
    while (i <= 5) {
        total = total + i;
        i++;
    }
    printf("Sum 1 to 5 = ");
    printf(total);
    printf("\n");

    for (int j = 1; j <= 4; j++) {
        printf("j = ");
        printf(j);
        printf("\n");
    }

    return 0;
}
