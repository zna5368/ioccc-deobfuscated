#include <stdlib.h>
#include <stdio.h>

/* (a + b) mod n */
int add_mod(int a, int b, int n)
{
	return (a + b) % n;
}

/* (a * b) mod n using binary double-and-add */
int mul_mod(int a, int b, int n)
{
	int r;

	if (b == 0)
		return 0;

	r = mul_mod(a, b >> 1, n);
	r = add_mod(r, r, n);
	if (b % 2)
		r = add_mod(a, r, n);
	return r;
}

/*
 * base^exp mod n using binary square-and-multiply, with a twist used for
 * the strong probable prime test: once a partial result reaches n-1 (that is,
 * -1) it stays there, and a partial result of 1 after an odd step is turned
 * into n-1.
 */
int pow_mod(int base, int exp, int n)
{
	int r;

	if (exp == 0)
		return 1;

	r = pow_mod(base, exp >> 1, n);
	if (exp % 2) {
		r = mul_mod(r, r, n);
		r = mul_mod(base, r, n);
		return r == 1 ? n - 1 : r;
	}
	return r == n - 1 ? n - 1 : mul_mod(r, r, n);
}

int main(int argc, char **argv)
{
	int base = 2;
	int n, half;

	if (argc >= 2)
		base = atoi(argv[1]);

	for (n = 3, half = 1;; n += 2, half++) {
		if (pow_mod(base, half, n) == n - 1) {
			printf("%d\n", n);
			fflush(stdout);
		}
	}
}
