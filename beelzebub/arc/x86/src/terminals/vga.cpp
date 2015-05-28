#include <terminals/vga.hpp>

namespace Beelzebub { namespace Terminals { namespace Vga {
	/******************
		Cell struct
	******************/

	/*	Constructors	*/
		
	Cell::Cell()
		: Character(0), Colors(DefaultColor)
	{
		
	}
		
	Cell::Cell(char const c, Color16 const fg, Color16 const bg)
		: Character(c), Colors(fg | (bg << 4))
	{
		
	}

}}}
