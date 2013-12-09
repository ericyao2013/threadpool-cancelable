#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "threadpool.h"

#define PRINT(x)	write(1, (x), sizeof(x) - 1)
#define PRINTERR(x)	write(2, (x), sizeof(x) - 1)

static void
on_init (void)
{
	PRINTERR("on_init: initializing\n");
}

static void
routine (void *data)
{
	write(1, data, strlen(data));

	// Hang around long enough to demonstrate the blocking effect:
	sleep(2);
}

static void
on_cancel (void)
{
	PRINT("on_cancel: oh dear!\n");
}

static void
on_exit (void)
{
	PRINT("on_exit: exiting\n");
}

static void
enqueue (struct threadpool *tp, char *msg)
{
	if (!threadpool_job_enqueue(tp, msg)) {
		PRINTERR("failed to enqueue!\n");
	}
}

int
main (void)
{
	struct threadpool *tp;

	// Create a threadpool with five threads waiting for work:
	if ((tp = threadpool_create(5, on_init, NULL, routine, on_cancel, on_exit)) == NULL) {
		PRINTERR("Could not create threadpool!\n");
		return 1;
	}
	// The first five tasks should print immediately, but the rest should
	// block till the first five reach the end of their sleep. Note that
	// due to scheduling issues, it is not guaranteed that the messages
	// print in order:
	enqueue(tp, "1. hello\n");
	enqueue(tp, "2. world\n");
	enqueue(tp, "3. this\n");
	enqueue(tp, "4. is\n");
	enqueue(tp, "5. a...\n");

	// Let these jobs print their messages and sleep inside their main
	// routine, then kill off one of the jobs prematurely to make room
	// for a new one:
	sleep(1);
	threadpool_job_cancel(tp, 4);

	// Try to enqueue a new job, which will (likely) fail for now because
	// of a race condition with the canceling thread (this is a TODO):
	enqueue(tp, "4a. new\n");
	sleep(3);

	// These should block till the first five tasks are enqueued:
	threadpool_job_enqueue(tp, "6. blocking\n");
	threadpool_job_enqueue(tp, "7. threadpool\n");

	PRINT("here's the main routine\n");
	sleep(3);

	// This will block till the threads are done (it won't kill threads):
	threadpool_destroy(&tp);

	return 0;
}
