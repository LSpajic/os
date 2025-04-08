#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

#define MAX_SIZE 8

typedef struct Stack
{
    int arr[MAX_SIZE];
    int top;
} Stack;

// Function DECLARATIONS
void initialize(Stack *stack);
bool isEmpty(Stack *stack);
bool isFull(Stack *stack);
void push(Stack *stack, int value);
int pop(Stack *stack);
int peek(Stack *stack);
int printStack(Stack *stack);
void binprintf(int n);

#endif