#include <synchronization/spinlock_uninterruptible.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    SpinlockUninterruptible<false>::~SpinlockUninterruptible()
    #else
    template<bool SMP>
    SpinlockUninterruptible<SMP>::~SpinlockUninterruptible()
    #endif
    {
        assert(this->Check(), "Spinlock (uninterruptible) @ %Xp was destructed while busy!", this);

        //this->Release();
    }//*/
#endif

template struct SpinlockUninterruptible<true>;
template struct SpinlockUninterruptible<false>;
