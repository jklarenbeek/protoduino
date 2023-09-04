#include "quicksort8.h"

void swap8x8(int8_t *arr, int_fast8_t i, int_fast8_t j)
{
    int_fast8_t t = arr[i];
    arr[i] = arr[j];
    arr[j] = t;
}

int_fast8_t partition8x8(int8_t *arr, int_fast8_t l, int_fast8_t r)
{
    int_fast8_t x = arr[r], i = l;
    for (int_fast8_t j = l; j <= r - 1; j++)
    {
        if (arr[j] <= x)
        {
            swap8x8(arr, i, j);
            i++;
        }
    }
    swap8x8(arr, i, r);
    return i;
}

void quickSort8x8(int8_t *arr, int_fast8_t l, int_fast8_t r)
{
    if (l < r)
    {
        // Parition the array around a pivot and get index of pivot
        int_fast8_t p = partition8x8(arr, l, r);
        // Recursively sort elements before and after pivot
        quickSort8x8(arr, l, p -1);
        quickSort8x8(arr, p + 1, r);
    }
}
