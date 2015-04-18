#include <terminals/base.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*********************************
	TerminalCoordinates struct
*********************************/

inline __bland TerminalCoordinates TerminalCoordinates::operator+(const TerminalCoordinates other)
{
	return { (int16_t)(this->X + other.X), (int16_t)(this->Y + other.Y) };
}

inline __bland TerminalCoordinates TerminalCoordinates::operator-(const TerminalCoordinates other)
{
	return { (int16_t)(this->X - other.X), (int16_t)(this->Y - other.Y) };
}
