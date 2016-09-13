#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>

#define GUARD(FOO) do { \
    spinlock(&lock); \
    FOO;\
    spinunlock(&lock); \
} while(0);

static int lock = 0;

static void spinlock(volatile int* lock)
{
    while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
        sched_yield();
    }
}

static void spinunlock(volatile int* lock)
{
    *lock = 0;
}


typedef struct node {
    int data;
    struct node* next;
} node;

typedef struct stack {
    int size;
    node* head;
} stack;

static void initStack(stack** st)
{
    stack* s = (stack*) malloc(sizeof(stack));
    if (s) {
        s->size = 0;
        s->head = NULL;
        (*st) = s;
    }
}

static void push(int data, stack* const st)
{
    node* t = (node*) malloc(sizeof(node));
    t->data = data;
    t->next = st->head;
    st->head = t;
}


int pop(stack* const st)
{
    // this is importnant to check by multiple threads
    // to try to enter in ABA problem
    if (st->head) {
        int ret = st->head->data;
        node* del = st->head;
        st->head = st->head->next;
        free(del);
        return ret;
    }
}


static void* cb(void* usr)
{
    stack* pstack = (stack*) usr;
    spinlock(&lock);
    printf("Pop: [%d]\n", pop(pstack));
    spinunlock(&lock);
}


int main(int argc, char *argv[])
{

    stack *pstack;
    initStack(&pstack);
    int i;
    for(i=0; i < 10000; i++) {
        push(i, pstack);
    }

    pthread_t threads[10000];

    for(i=0; i < 10000; i++) {
        pthread_create(&threads[i], NULL, cb, (stack*)pstack);
    }

    for(i=0; i < 10000; i++) {
        pthread_join(&threads[i], NULL);
    }

    return 0;
}
