#include <synchronization/rw_spinlock.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
/*RwSpinlock::~RwSpinlock()
{
    //assert(this->Check(), "Spinlock (uninterruptible) @ %Xp was destructed while busy!", this);

    //this->Release();
}//*/
#endif
