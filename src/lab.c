#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h> /* for gettimeofday system call */
#include "lab.h"

struct parallel_args *sorters[MAX_THREADS];

/**
 * @brief Standard insertion sort that is faster than merge sort for small array's
 *
 * @param A The array to sort
 * @param p The starting index
 * @param r The ending index
 */
static void insertion_sort(int A[], int p, int r)
{
  int j;

  for (j = p + 1; j <= r; j++)
    {
      int key = A[j];
      int i = j - 1;
      while ((i > p - 1) && (A[i] > key))
        {
	  A[i + 1] = A[i];
	  i--;
        }
      A[i + 1] = key;
    }
}


void mergesort_s(int A[], int p, int r)
{
  if (r - p + 1 <=  INSERTION_SORT_THRESHOLD)
    {
      insertion_sort(A, p, r);
    }
  else
    {
      int q = (p + r) / 2;
      mergesort_s(A, p, q);
      mergesort_s(A, q + 1, r);
      merge_s(A, p, q, r);
    }

}

void merge_s(int A[], int p, int q, int r)
{
  int *B = (int *)malloc(sizeof(int) * (r - p + 1));

  int i = p;
  int j = q + 1;
  int k = 0;
  int l;

  /* as long as both lists have unexamined elements */
  /*  this loop keeps executing. */
  while ((i <= q) && (j <= r))
    {
      if (A[i] < A[j])
        {
	  B[k] = A[i];
	  i++;
        }
      else
        {
	  B[k] = A[j];
	  j++;
        }
      k++;
    }

  /* now only at most one list has unprocessed elements. */
  if (i <= q)
    {
      /* copy remaining elements from the first list */
      for (l = i; l <= q; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }
  else
    {
      /* copy remaining elements from the second list */
      for (l = j; l <= r; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }

  /* copy merged output from array B back to array A */
  k = 0;
  for (l = p; l <= r; l++)
    {
      A[l] = B[k];
      k++;
    }

  free(B);
}

void mergesort_mt(int *A, int n, int num_thread) {
  if(MAX_THREADS < num_thread) num_thread = MAX_THREADS; // on the off chance number of threads is greater than permitted
  if(n < num_thread) num_thread = n; //on the off chance the number of threads is bigger than the size of the array

  //calculate increment, find out how many threads need to get an extra piece, and set start
  int inc = n / num_thread;
  int mod = n % num_thread;
  int start = 0;

  for(int i = 0; i < num_thread; i++) {
    sorters[i] = (struct parallel_args *)malloc(sizeof(struct parallel_args));
    sorters[i]->A = A;
    sorters[i]->start = start;
    //end = start + inc -1 since mergesort_s is end inclusive, then + 1 if there are still modulus to dole out
    sorters[i]->end = start + inc + ((mod > 0) ? 0 : -1); 

    if(pthread_create(&(sorters[i]->tid), NULL, parallel_mergesort, sorters[i]) != 0) {
      perror("Error creating thread!");
      for(int j = i; j >= 0; j--) free(sorters[j]);
      free(A);
      exit(EXIT_FAILURE);
    }

    if(mod > 0) mod--; //decrement mod to ensure the proper number of leftovers are handed out

    start = sorters[i]->end + 1; //increment start for the next increment, end + 1 since mergesort_s is end inclusive
  }

  for(int i = 0; i < num_thread; i++) {
    pthread_join(sorters[i]->tid, NULL);
  }

  //time to finish sorting
  for(int i = 0; i < n; i++) {
    int min = i; //min index location, starting at i
    int s = 0; //min sorters index

    //A[i] within a partition will be the min value for that position, as such start with j = 1 since A[i] is already the smallest value within sorters[0]
    for(int j = 1; j < num_thread; j++) { 
      if(sorters[j]->start > i) { 
        if(A[sorters[j]->start] < A[min]) {
          min = sorters[j]->start;
          s = j;
        }
      }
    }

    //if a new min was found, move it to i, then bubble sort the original value of A[i] within its new partition
    if(min != i) {
      int temp = A[i];
      A[i] = A[min];
      //if min has reached the end of the partition we've searched the whole partition, and we already know temp > A[min], so start at min + 1 
      while(min < sorters[s]->end && temp > A[min + 1]) { 
        A[min] = A[min + 1]; //since temp is also larger than min + 1, move min + 1 down
        min++;
      }
      A[min] = temp;
    }
  }

  //now that sorting is done, start freeing stuff
  for(int i = 0; i < num_thread; i++) free(sorters[i]);

  //debugging
  //for(int i = 0; i < n; i++) printf("%d, ", A[i]);
  //printf("\n");
}

double getMilliSeconds()
{
  struct timeval now;
  gettimeofday(&now, (struct timezone *)0);
  return (double)now.tv_sec * 1000.0 + now.tv_usec / 1000.0;
}

void *parallel_mergesort(void *args) {
  if(args == NULL) {perror("Null args passed to parallel mergesort!"); exit(EXIT_FAILURE);}
  struct parallel_args *arg = args; //cast args to arg to easier access
  
  mergesort_s(arg->A, arg->start, arg->end); //make use of the existing mergsort function

  pthread_exit(NULL); //thread complete
}
