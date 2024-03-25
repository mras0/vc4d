void __cxa_pure_virtual(void) { while (1) ; }

int __cxa_guard_acquire(char *guard_object)
{
    return *guard_object == 0; // Return 1 if the initialization is not yet complete; 0 otherwise.
}

void __cxa_guard_release(char* guard_object)
{
    *guard_object = 1;
}
