/*
Stackless coroutines need a compiler in order to be generic:

The compiler rewrites the implicit
state machine contained in an async-block into an explicit state
machine and uses that code to implement the Future-interface for
a compiler-generated, anonymous structure holding the state of
the local variables and the program counter. The user transfers
ownership of a Future to an Executor that implements a loop over
dynamically dispatched implementors of Future and usually also
allows them to queue additional implementors of Future through its
specific interface.

Upon encountering an async-block, the compiler:
(1) identifies all local variables in scope of the async-block and
generates a new, anonymous structure which includes them
and the current state of the program counter as attributes,
(2) generates code at the start of the function body to resume at
the previous suspension point using a computed goto based
on the state of the program counter,
(3) implements trait Future for this new structure to allow it
to be recursively awaited from other futures:
• by generating a suspension point for each invokation of
await that manifests as code polling the Future to determine whether it is ready or still pending,
• if it is still pending, by storing the current state of the
program counter in the anonymous instance of the new
structure and leaving the function.
*/

#include <stdio.h>
#include <unistd.h>

struct task {
    void (*coroutine) (void*, void*);
    void *state;
    void *channel;
};

struct co_write_number_state {
    int program_counter;
    int number;
};

struct co_read_number_state {
    int program_counter;
};

struct number_channel {
    int number;
};

void co_write_number(void *co_write_number_state, void *write_channel) {
    switch (((struct co_write_number_state*)co_write_number_state)->program_counter) {
        case 0:
            while (1) {
                ((struct co_write_number_state*)co_write_number_state)->program_counter = 1;
                ((struct number_channel*)write_channel)->number = ((struct co_write_number_state*)co_write_number_state)->number;
                return;
        case 1:
            ((struct co_write_number_state*)co_write_number_state)->number++;
            }
    }
}

void co_read_number(void *co_read_number_state, void *read_channel) {
    switch (((struct co_read_number_state*)co_read_number_state)->program_counter) {
        case 0:
            while (1) {
                ((struct co_read_number_state*)co_read_number_state)->program_counter = 1;
                printf("Read number %d\n", ((struct number_channel*)read_channel)->number);
                return;
        case 1:
            }
    }
}

void schedule(struct task *tasks, int len) {
    int co_index = 0;
    struct task current_task;

    while (1) {
        current_task = tasks[co_index];
        printf("current task %d\n", co_index);

        current_task.coroutine(current_task.state, current_task.channel);
        co_index = (co_index + 1) % len;
        sleep(1);
    }
}

int main() {
    struct co_write_number_state w_state = {0, 0};
    struct co_read_number_state r_state = {0};
    struct number_channel chan = {0};
    struct task tasks[2] = {
        {co_write_number, &w_state, &chan},
        {co_read_number, &r_state, &chan}
    };

    schedule(tasks, 2);

    return 0;
}
