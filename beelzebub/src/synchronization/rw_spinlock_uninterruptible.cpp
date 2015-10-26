#include <synchronization/rw_spinlock_uninterruptible.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
/*RwSpinlockUninterruptible::~RwSpinlockUninterruptible()
{
    //assert(this->Check(), "Spinlock (uninterruptible) @ %Xp was destructed while busy!", this);

    //this->Release();
}//*/
#endif
