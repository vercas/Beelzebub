#include <math.h>
#include <icxxabi.h>

extern "C" void __cxa_pure_virtual()
{
    //  NUTHIN
    // I should panic here.
}

extern "C" void doCppStuff()
{
    
}


void * operator new(size_t size) throw()
{
    return nullptr; // malloc(size);
}
 
void * operator new[](size_t size) throw()
{
    return nullptr; // malloc(size);
}
 
void operator delete(void *p) throw()
{
    //free(p);
}
 
void operator delete[](void *p) throw()
{
    //free(p);
}
