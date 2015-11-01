#include <synchronization/spinlock.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    Spinlock<false>::~Spinlock()
    #else
    template<bool SMP>
    Spinlock<SMP>::~Spinlock()
    #endif
    {
        assert(this->Check(), "Spinlock @ %Xp was destructed while busy!", this);

        //this->Release();
    }//*/
#endif

template struct Spinlock<true>;
template struct Spinlock<false>;
