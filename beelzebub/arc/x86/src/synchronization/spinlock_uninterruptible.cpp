#include <synchronization/spinlock_uninterruptible.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
SpinlockUninterruptible::~SpinlockUninterruptible()
{
	assert(this->Check(), "Spinlock (uninterruptible) @ %Xp was destructed while busy!", this);

	//this->Release();
}//*/
#endif
