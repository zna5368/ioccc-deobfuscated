/*
 * Number of disks, normally given on the compile line with -Dn=LEVEL.
 */
#ifndef n
#define n 05
#endif

#define DISKS n
#define FORMAT_STRING "Move disk %d from peg %d to peg %d\n"

/*
 * Move a tower of `disks` disks from peg `from` to peg `to`,
 * using peg `spare` as temporary storage.
 */
void hanoi(int disks, int from, int to, int spare)
{
    if (disks == 0)
        return;

    hanoi(disks - 1, from, spare, to);
    printf(FORMAT_STRING, disks, from, to);
    hanoi(disks - 1, spare, to, from);
}

main()
{
    /*
     * An odd number of disks ends on peg 2 and an even number
     * of disks ends on peg 3.
     */
    if (DISKS % 2)
        hanoi(DISKS, 1, 2, 3);
    else
        hanoi(DISKS, 1, 3, 2);
}
