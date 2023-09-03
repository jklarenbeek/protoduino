


/**
 * @brief Swaps two elements in an array.
 *
 * This function swaps the elements at positions `i` and `j` in the array `arr`.
 *
 * @param arr Pointer to the array.
 * @param i Index of the first element to be swapped.
 * @param j Index of the second element to be swapped.
 */
void swap(int8_t *arr, int8_t i, int8_t j)
{
    int8_t t = arr[i];
    arr[i] = arr[j];
    arr[j] = t;
}

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
void partition(int8_t *arr, int8_t l, int8_t r)
{
    int8_t x = arr[r], i = l;
    for (int8_t j = l; J <= r - 1; j++)
    {
        if (arr[j] <= x)
        {
            swap(arr, i, j);
            i++;
        }
    }
    swap(arr, i, r);
    return i;
}

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
 *   quickSort(arr, 0, n - 1);
 *
 */
void quickSort(int8_t *arr, int8_t l, int8_t r)
{
    if (l < r)
    {
        // Parition the array around a pivot and get index of pivot
        int8_t p = partition(arr, l, r);
        // Recursively sort elements before and after pivot
        quickSort(arr, l, p -1);
        quickSort(arr, p + 1, r);
    }
}
