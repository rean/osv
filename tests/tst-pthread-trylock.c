#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

pthread_mutex_t mtx;
volatile bool quit = false;
pthread_t t1;

unsigned int tests_total = 0, tests_failed = 0;

void report(const char* name, bool passed)
{
   static const char* status[] = {"FAIL", "PASS"};
   printf("%s: %s\n", status[passed], name);
   tests_total += 1;
   tests_failed += !passed;
}

void* critical_section(void* ignore)
{
   pthread_mutex_lock(&mtx);
   while (!quit) {
      ; // Spin (waste cycles)
   }
   pthread_mutex_unlock(&mtx);
   return NULL;
}

int main(void)
{
   printf("Starting pthread_mutex_trylock test\n");

   // Basic flow for test:
   // 1) thread1 grabs the mutex and in the critical section waits for a
   // global variable to change
   // 2) main thread does a try lock, expecting EBUSY
   // 3) main thread updates the global variable so thread1 exits

   pthread_mutex_init(&mtx, NULL);
   // Start thread t1, which will immediately try to grab the mutex, it
   // should succeed
   printf("[Main thread] starting new thread: thread1\n");
   pthread_create(&t1, NULL, critical_section, NULL);
   // Put the main thread to sleep for a few seconds, this should be enough
   // time to allow t1 to get the mutex
   sleep(10);
   // Now the main thread will try to grab the mutex - it should be already
   // locked by t1
   printf("[Main thread] trying to acquire lock\n");
   int retval = pthread_mutex_trylock(&mtx);
   printf("[Main thread] pthread_mutex_trylock retval: %d\n", retval);
   // If we happened to get the mutex - that's unexpected
   report("pthread_try_lock\0", retval == EBUSY);
   if (retval == 0) {
      pthread_mutex_unlock(&mtx);
   } else if (retval != EBUSY) {
      printf("[Main thread] Expected EBUSY (%d) but got %d instead\n",
             EBUSY, retval);
   }
   // Update the global variable (flag) so thread1 exits the critical section
   quit = true;
   printf("[Main thread] Freeing thread1 then joining...\n");
   pthread_join(t1, NULL);
   printf("SUMMARY: %u tests / %u failures\n", tests_total, tests_failed);
   return tests_failed == 0 ? 0 : 1;
}
