# Cancelable threadpool

A cancelable threadpool is a threadpool that lets you cancel running jobs, freeing threads for new work.
An example use case is a map viewer that downloads many tiles concurrently.
When the user scrolls the map, the threadpool should abort all running download jobs to make room for the new set of tiles that need to be downloaded.

At threadpool creation time, the user passes a number of callbacks that are called when various events happen, as explained below.
When the user asks the threadpool to cancel a job, the threadpool calls the user's `on_cancel` callback.
This callback takes some user-defined action that asks the main routine to quit, such as setting a loop flag or sending a byte with the self-pipe trick.
The thread will then slip back into the pool, and a slot will open for a new job to be enqueued.

This library is written and tested under Linux.
It uses the regular `pthread` primitives, and does notifications with signals (`SIGUSR1` to be precise).
It's not terribly battle-tested and can probably be improved in many ways.
Should not be used in production yet.

## License

This project is licensed under the [BSD 3-clause license](http://opensource.org/licenses/BSD-3-Clause).

## Methods

Taken from `threadpool.h`:

```c
struct threadpool *threadpool_create
(
 	// Number of threads to create:
	size_t nthreads,

	// Run once when thread starts up:
	void (*on_init)(void),

	// Run when data is dequeued without being processed:
	void (*on_dequeue)(void *data),

	// Main thread routine:
	void (*routine)(void *data),

	// Run in signal handler when thread is canceled:
	void (*on_cancel)(void),

	// Run once when thread shuts down:
	void (*on_exit)(void)
);
```

Create the threadpool.
Returns a pointer to the allocated threadpool structure on success, `NULL` on failure.

```c
int threadpool_job_enqueue (struct threadpool *const p, void *const data);
```

Enqueue the job specified by the opaque data pointer into the threadpool.
Returns a job id larger than 0 if enqueuement succeeded, or 0 on failure.

```c
bool threadpool_job_cancel (struct threadpool *const p, int job_id);
```

Cancel the job with the given ID by signaling the thread.
The signal handler routine calls the user-provided `on_cancel()` function, which is responsible for taking some action that causes the main routine to quit.
This function does not wait for the main function to quit, so upon return there is no guarantee that a job slot is free.
Returns `true` on success, `false` on error.

```c
void threadpool_destroy (struct threadpool **const p);
```

Destroy the threadpool structure and all associated resources.

## Callbacks

These callbacks are provided by the user, and passed on calling `threadpool_create`.

```c
void (*on_init)(void)
```

This routine is run once when the thread is started.
Can be used to initialize thread-specific data.
May be given as `NULL`, in which case nothing happens.

```c
void (*on_dequeue)(void *data)
```

This routine is run when the data pointed to by `*data` is dequeued from the job queue before been processed by a thread.
Can be used to free the pointer.
May be `NULL`.

```c
void (*routine)(void *data)
```

This is the thread's main routine.
A thread "runs" a job by calling this callback with the data pointer provided by `threadpool_job_enqueue` as an argument.
What the routine does is up to the user.
The routine should check a loop flag or some other marker as set by `on_cancel` to see if it has been asked to abort its task.

```c
void (*on_cancel)(void)
```

This routine is called when the user calls `threadpool_job_cancel` for a job that a thread is running.
(If the job is not yet running, `on_dequeue` will be called.)
The routine runs inside a mutex, so should be short and sweet.
Its purpose is to set some kind of condition that tells the main routine that it should abort.
May be `NULL`, to defeat the purpose of a cancelable threadpool.

```c
void (*on_exit)(void)
```

This routine is run once when the threadpool is destroyed.
Can be used to free thread-specific data.
May be `NULL`.

## Usage example

See `example.c` for a real working example.
Basically, include `threadpool.h` at the top of your code.
Create a threadpool of the desired size, passing in your callback functions.
Enqueue jobs, saving the job number somewhere.
When you need to shoot down a job, call `threadpool_job_cancel` on the job number.
A job slot will open in the near future (depending on how fast your main routine exits), so there will hopefully be room when you retry the enqueuement.
