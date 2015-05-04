#include <arc/synchronization/spinlock.hpp>

using namespace Beelzebub::Synchronization;

Spinlock::~Spinlock()
{
	this->Release();
}//*/
