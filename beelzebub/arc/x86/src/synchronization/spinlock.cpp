#include <arc/synchronization/spinlock.hpp>

using namespace Beelzebub::Synchronization;

#ifdef __BEELZEBUB__DEBUG
Spinlock::~Spinlock()
{
	assert(this->Check(), "Spinlock @ %Xp was destructed while busy!", this);

	//this->Release();
}//*/
#endif
