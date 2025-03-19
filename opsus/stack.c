#include "stack.h"
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

void binprintf(int v)
{
    unsigned int mask = 1 << ((sizeof(int) << 3) - 1);
    mask >>= 27;
    while (mask)
    {
        printf("%d", (v & mask ? 1 : 0));
        mask >>= 1;
    }
}

void initialize(Stack *stack)
{
    stack->top = -1;
}

bool isEmpty(Stack *stack)
{
    return stack->top == -1;
}

bool isFull(Stack *stack)
{
    return stack->top == MAX_SIZE - 1;
}

void push(Stack *stack, int value)
{
    if (isFull(stack))
    {
        printf("Stack Overflow\n");
        return;
    }

    stack->arr[++stack->top] = value;
    printf("Pushed: ");
    binprintf(value);
    printf("\n");
}

int pop(Stack *stack)
{
    if (isEmpty(stack))
    {
        printf("Stack Underflow\n");
        return -1;
    }
    int popped = stack->arr[stack->top--];
    // printf("Popped %d\n", popped);
    return popped;
}

int peek(Stack *stack)
{
    if (isEmpty(stack))
    {
        // printf("Stack empty\n");
        return -1;
    }
    return stack->arr[stack->top];
}

int printStack(Stack *stack)
{
    printf("Stack contents: ");
    if (isEmpty(stack))
    {
        // printf("Stack empty\n");
        return -1;
    }
    for (int i = 0; i <= stack->top; i++)
    {
        binprintf(stack->arr[i]);
        printf(";");
    }
    printf("\n");
    return 0;
}