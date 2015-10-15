#include <synchronization/spinlock.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
template<bool SMP>
Spinlock<SMP>::~Spinlock()
{
    assert(this->Check(), "Spinlock @ %Xp was destructed while busy!", this);

    //this->Release();
}//*/
#endif

template struct Spinlock<true>;
template struct Spinlock<false>;
