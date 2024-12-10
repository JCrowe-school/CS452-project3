#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
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

//parallel merging
struct parallel_margs {
  struct parallel_args *args;
  int mid;
};

void *parallel_merge(void *args) {
  if(args == NULL) {fprintf(stderr, "Null args passed to parallel merge!"); exit(EXIT_FAILURE);}
  struct parallel_margs *arg = args; //cast args to arg to easier access
  
  merge_s(arg->args->A, arg->args->start, arg->mid, arg->args->end); //make use of the existing mergsort function

  pthread_exit(NULL); //thread complete
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
      for(int j = i; j >= 0; j--) {
        pthread_cancel(sorters[j]->tid); //cancel allocated threads
        free(sorters[j]); //free all sorters
      }
      free(A);
      exit(EXIT_FAILURE);
    }

    if(mod > 0) mod--; //decrement mod to ensure the proper number of leftovers are handed out

    start = sorters[i]->end + 1; //increment start for the next increment, end + 1 since mergesort_s is end inclusive
  }

  for(int i = 0; i < num_thread; i++) {
    pthread_join(sorters[i]->tid, NULL);
  }

  //time to finish merging
  int shift = 1;
  shift = shift << 1;
  static struct parallel_margs arg;
  while(num_thread != 1 && num_thread >= shift) {

    //loop over the partitions in such a way to be able to merge adjacent partitions, then pass the joined partition to a thread
    for(int i = 0; i + (shift / 2) < num_thread; i = i + shift) {
      arg.args = sorters[i];
      arg.mid = sorters[i]->end;
      arg.args->end = sorters[i + (shift / 2)]->end; //merge by setting the end of the first partition to the end of the second

      if(pthread_create(&(arg.args->tid), NULL, parallel_merge, &arg) != 0) {
        perror("Error creating thread!");
        for(int j = 0; j < i; j = j + shift) pthread_cancel(sorters[i]->tid); //cancel created threads
        for(int j = num_thread - 1; j >= 0; j--) free(sorters[j]); //free all sorters
        free(A);
        exit(EXIT_FAILURE);
      }
    }

    for(int i = 0; i < num_thread / shift; i = i + shift) {
      pthread_join(sorters[i]->tid, NULL);
    }

    //shift for next loop
    shift = shift << 1;
  }
  if(num_thread != 1 && num_thread % 2 == 1) { //if num_thread is odd, threads will have one unmerged partition left
    arg.args = sorters[0];
    arg.mid = sorters[0]->end; 
    arg.args->end = sorters[num_thread - 1]->end;
    if(pthread_create(&(arg.args->tid), NULL, parallel_merge, &arg) != 0) {
      perror("Error creating thread!");
      for(int j = num_thread - 1; j >= 0; j--) free(sorters[j]); //free all sorters
      free(A);
      exit(EXIT_FAILURE);
    }

    pthread_join(arg.args->tid, NULL);
  }

  //debugging
  //printf("[");
  //for(int i = 0; i < n; i++) {
  //  printf("%d, ", A[i]);
  //}
  //printf("]\n");

  //now that sorting is done, start freeing stuff
  for(int i = 0; i < num_thread; i++) free(sorters[i]);
}

double getMilliSeconds()
{
  struct timeval now;
  gettimeofday(&now, (struct timezone *)0);
  return (double)now.tv_sec * 1000.0 + now.tv_usec / 1000.0;
}

void *parallel_mergesort(void *args) {
  if(args == NULL) {fprintf(stderr, "Null args passed to parallel mergesort!"); exit(EXIT_FAILURE);}
  struct parallel_args *arg = args; //cast args to arg to easier access
  
  mergesort_s(arg->A, arg->start, arg->end); //make use of the existing mergsort function

  pthread_exit(NULL); //thread complete
}
