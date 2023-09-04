
#ifndef QUICKSORT8_H
#define QUICKSORT8_H

#include "../sys/cc.h"
#include <stdint.h>

/**
 * @brief Swaps two elements in an array.
 *
 * This function swaps the elements at positions `i` and `j` in the array `arr`.
 *
 * @param arr Pointer to the array.
 * @param i Index of the first element to be swapped.
 * @param j Index of the second element to be swapped.
 */
CC_EXTERN void swap8x8(int8_t *arr, int_fast8_t i, int_fast8_t j);

/**
 * @brief Partitions an array around a pivot.
 *
 * This function partitions the array `arr` around the rightmost element as the pivot.
 * It returns the index of the pivot element after partitioning.
 *
 * @param arr Pointer to the array to be partitioned.
 * @param l Index of the leftmost element in the current partition.
 * @param r Index of the rightmost element in the current partition.
 * @return Index of the pivot element after partitioning.
 */
CC_EXTERN int_fast8_t partition8x8(int8_t *arr, int_fast8_t l, int_fast8_t r);

/**
 * @brief Sorts an array using the Quicksort algorithm.
 *
 * This function recursively sorts the elements in the array `arr` from index `l` to `r`
 * using the Quicksort algorithm.
 *
 * @param arr Pointer to the array to be sorted.
 * @param l Index of the leftmost element in the current partition.
 * @param r Index of the rightmost element in the current partition.
 * 
 * @example
 *   int8_t arr[] = {64, 34, 25, 12, 22, 11, 90};
 *   int8_t n = sizeof(arr) / sizeof(arr[0]);
 *
 *   quickSort8x8(arr, 0, n - 1);
 *
 */
CC_EXTERN void quickSort8x8(int8_t *arr, int_fast8_t l, int_fast8_t r);

#endif
