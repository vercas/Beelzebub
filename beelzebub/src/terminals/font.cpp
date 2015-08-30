#include <terminals/font.hpp>

const uint8_t Beelzebub::Terminals::FontWidth = 8;
const uint8_t Beelzebub::Terminals::FontHeight = 14;

const uint8_t Beelzebub::Terminals::FontMin = 33;
const uint8_t Beelzebub::Terminals::FontMax = 126;

const uint8_t Beelzebub::Terminals::FontUnderlineHeight = Beelzebub::Terminals::FontHeight - 2;

/**
 * The font I am attempting to implement is "Free Pixel", obtained here:
 * http://www.dafont.com/free-pixel.font?fpp=100&l[]=10&l[]=1
 *
 * The lowercase letters have been implemented by my comrade Vaxe:
 * http://steamcommunity.com/id/sVaxe/
 **/

const uint8_t Beelzebub::Terminals::Font[][14] = {
	{   //   33 | 0x21 | !
		b(00000000),
		b(00000000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00000000),
	},
	{   //   34 | 0x22 | "
		b(00000000),
		b(01100110),
		b(01100110),
		b(01100110),
		b(01100110),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   35 | 0x23 | #
		b(00000000),
		b(00000000),
		b(00100100),
		b(00100100),
		b(01111110),
		b(00100100),
		b(00100100),
		b(00100100),
		b(00100100),
		b(01111110),
		b(00100100),
		b(00100100),
		b(00000000),
		b(00000000),
	},
	{   //   36 | 0x24 | $
		b(00000000),
		b(00010000),
		b(01111100),
		b(10010010),
		b(10010000),
		b(10010000),
		b(01111100),
		b(00010010),
		b(00010010),
		b(00010010),
		b(10010010),
		b(01111100),
		b(00010000),
		b(00000000),
	},
	{   //   37 | 0x25 | %
		b(00000000),
		b(00000000),
		b(01100000),
		b(10010000),
		b(10010010),
		b(01100100),
		b(00001000),
		b(00010000),
		b(00100000),
		b(01001100),
		b(10010010),
		b(00010010),
		b(00001100),
		b(00000000),
	},
	{   //   38 | 0x26 | &
		b(00000000),
		b(00000000),
		b(00011000),
		b(00100100),
		b(00100100),
		b(00011000),
		b(00110000),
		b(01001010),
		b(01001010),
		b(01000100),
		b(01000100),
		b(00111010),
		b(00000000),
		b(00000000),
	},
	{   //   39 | 0x27 | '
		b(00000000),
		b(00000000),
		b(00110000),
		b(00110000),
		b(00110000),
		b(00110000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   40 | 0x28 | (
		b(00000000),
		b(00001000),
		b(00010000),
		b(00010000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00010000),
		b(00010000),
		b(00001000),
		b(00000000),
	},
	{   //   41 | 0x29 | )
		b(00000000),
		b(00100000),
		b(00010000),
		b(00010000),
		b(00001000),
		b(00001000),
		b(00001000),
		b(00001000),
		b(00001000),
		b(00001000),
		b(00010000),
		b(00010000),
		b(00100000),
		b(00000000),
	},
	{   //   42 | 0x2A | *
		b(00000000),
		b(00000000),
		b(00010000),
		b(01010100),
		b(00111000),
		b(01010100),
		b(00010000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   43 | 0x2B | +
		b(00000000),
		b(00000000),
		b(00000000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(11111110),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   44 | 0x2C | ,
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011000),
		b(00011000),
		b(00011000),
		b(00110000),
		b(00000000),
	},
	{   //   45 | 0x2D | -
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(11111110),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   46 | 0x2E | .
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011000),
		b(00011000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   47 | 0x2F | /
		b(00000000),
		b(00000010),
		b(00000010),
		b(00000100),
		b(00000100),
		b(00001000),
		b(00001000),
		b(00010000),
		b(00010000),
		b(00100000),
		b(00100000),
		b(01000000),
		b(01000000),
		b(00000000),
	},
	{   //   48 | 0x30 | 0
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(01000110),
		b(01001010),
		b(01010010),
		b(01100010),
		b(01000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
	},
	{   //   49 | 0x31 | 1
		b(00000000),
		b(00000000),
		b(00010000),
		b(00110000),
		b(01010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(01111100),
		b(00000000),
		b(00000000),
	},
	{   //   50 | 0x32 | 2
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(00000010),
		b(00000100),
		b(00001000),
		b(00010000),
		b(00100000),
		b(01000000),
		b(01111110),
		b(00000000),
		b(00000000),
	},
	{   //   51 | 0x33 | 3
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(00000010),
		b(00000100),
		b(00011000),
		b(00000100),
		b(00000010),
		b(00000010),
		b(01000100),
		b(00111000),
		b(00000000),
		b(00000000),
	},
	{   //   52 | 0x34 | 4
		b(00000000),
		b(00000000),
		b(00001100),
		b(00010100),
		b(00010100),
		b(00100100),
		b(00100100),
		b(01000100),
		b(01111110),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000000),
		b(00000000),
	},
	{   //   53 | 0x35 | 5
		b(00000000),
		b(00000000),
		b(01111110),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01111100),
		b(00000010),
		b(00000010),
		b(00000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
	},
	{   //   54 | 0x36 | 6
		b(00000000),
		b(00000000),
		b(00011100),
		b(00100000),
		b(01000000),
		b(01000000),
		b(01011100),
		b(01100010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
	},
	{   //   55 | 0x37 | 7
		b(00000000),
		b(00000000),
		b(01111110),
		b(01000010),
		b(00000010),
		b(00000100),
		b(00000100),
		b(00001000),
		b(00001000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00000000),
	},
	{   //   56 | 0x38 | 8
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(00100100),
		b(00011000),
		b(00100100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
	},
	{   //   57 | 0x39 | 9
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000110),
		b(00111010),
		b(00000010),
		b(00000010),
		b(00000100),
		b(00111000),
		b(00000000),
		b(00000000),
	},
	{   //   58 | 0x3A | :
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011000),
		b(00011000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011000),
		b(00011000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   59 | 0x3B | ;
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011000),
		b(00011000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011000),
		b(00011000),
		b(00011000),
		b(00110000),
		b(00000000),
	},
	{   //   60 | 0x3C | <
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000100),
		b(00001000),
		b(00010000),
		b(00100000),
		b(00010000),
		b(00001000),
		b(00000100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   61 | 0x3D | =
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01111110),
		b(00000000),
		b(01111110),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   62 | 0x3E | >
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00100000),
		b(00010000),
		b(00001000),
		b(00000100),
		b(00001000),
		b(00010000),
		b(00100000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   63 | 0x3F | ?
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(00000010),
		b(00000100),
		b(00001000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00010000),
		b(00000000),
		b(00000000),
	},
	{   //   64 | 0x40 | @
		b(00000000),
		b(00000000),
		b(00111000),
		b(01000100),
		b(01000010),
		b(10011010),
		b(10101010),
		b(10101010),
		b(10101010),
		b(10010100),
		b(01000000),
		b(01000100),
		b(00111000),
		b(00000000),
	},
	{   //   65 | 0x41 | A
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111110),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
	},
	{   //   66 | 0x42 | B
		b(00000000),
		b(00000000),
		b(01111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111100),
		b(00000000),
		b(00000000),
	},
	{   //   67 | 0x43 | C
		b(00000000),
		b(00000000),
		b(00011100),
		b(00100010),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(00100010),
		b(00011100),
		b(00000000),
		b(00000000),
	},
	{   //   68 | 0x44 | D
		b(00000000),
		b(00000000),
		b(01111000),
		b(01000100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000100),
		b(01111000),
		b(00000000),
		b(00000000),
	},
	{   //   69 | 0x45 | E
		b(00000000),
		b(00000000),
		b(01111110),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01111100),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01111110),
		b(00000000),
		b(00000000),
	},
	{   //   70 | 0x46 | F
		b(00000000),
		b(00000000),
		b(01111110),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01111100),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(00000000),
		b(00000000),
	},
	{   //   71 | 0x47 | G
		b(00000000),
		b(00000000),
		b(00011100),
		b(00100010),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000110),
		b(01000010),
		b(00100010),
		b(00011110),
		b(00000000),
		b(00000000),
	},
	{   //   72 | 0x48 | H
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111110),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
	},
	{   //   73 | 0x49 | I
		b(00000000),
		b(00000000),
		b(01111100),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(01111100),
		b(00000000),
		b(00000000),
	},
	{   //   74 | 0x4A | J
		b(00000000),
		b(00000000),
		b(00011110),
		b(00000010),
		b(00000010),
		b(00000010),
		b(00000010),
		b(00000010),
		b(00000010),
		b(00000010),
		b(01000100),
		b(00111000),
		b(00000000),
		b(00000000),
	},
	{   //   75 | 0x4B | K
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000100),
		b(01001000),
		b(01010000),
		b(01100000),
		b(01100000),
		b(01010000),
		b(01001000),
		b(01000100),
		b(01000010),
		b(00000000),
		b(00000000),
	},
	{   //   76 | 0x4C | L
		b(00000000),
		b(00000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01111110),
		b(00000000),
		b(00000000),
	},
	{   //   77 | 0x4D | M
		b(00000000),
		b(00000000),
		b(10000010),
		b(10000010),
		b(11000110),
		b(11000110),
		b(10101010),
		b(10101010),
		b(10010010),
		b(10010010),
		b(10000010),
		b(10000010),
		b(00000000),
		b(00000000),
	},
	{   //   78 | 0x4E | N
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(01100010),
		b(01010010),
		b(01010010),
		b(01001010),
		b(01001010),
		b(01000110),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
	},
	{   //   79 | 0x4F | O
		b(00000000),
		b(00000000),
		b(00011000),
		b(00100100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00100100),
		b(00011000),
		b(00000000),
		b(00000000),
	},
	{   //   80 | 0x50 | P
		b(00000000),
		b(00000000),
		b(01111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111100),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(00000000),
		b(00000000),
	},
	{   //   81 | 0x51 | Q
		b(00000000),
		b(00000000),
		b(00011000),
		b(00100100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01001010),
		b(00100100),
		b(00011010),
		b(00000000),
		b(00000000),
	},
	{   //   82 | 0x52 | R
		b(00000000),
		b(00000000),
		b(01111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111100),
		b(01001000),
		b(01000100),
		b(01000100),
		b(01000010),
		b(00000000),
		b(00000000),
	},
	{   //   83 | 0x53 | S
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000000),
		b(01000000),
		b(00111100),
		b(00000010),
		b(00000010),
		b(00000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
	},
	{   //   84 | 0x54 | T
		b(00000000),
		b(00000000),
		b(11111110),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00000000),
	},
	{   //   85 | 0x55 | U
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
	},
	{   //   86 | 0x56 | V
		b(00000000),
		b(00000000),
		b(10000010),
		b(10000010),
		b(10000010),
		b(01000100),
		b(01000100),
		b(01000100),
		b(00101000),
		b(00101000),
		b(00101000),
		b(00010000),
		b(00000000),
		b(00000000),
	},
	{   //   87 | 0x57 | W
		b(00000000),
		b(00000000),
		b(10000010),
		b(10000010),
		b(10000010),
		b(10000010),
		b(10010010),
		b(10010010),
		b(10101010),
		b(10101010),
		b(01000100),
		b(01000100),
		b(00000000),
		b(00000000),
	},
	{   //   88 | 0x58 | X
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(00100100),
		b(00100100),
		b(00011000),
		b(00011000),
		b(00100100),
		b(00100100),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
	},
	{   //   89 | 0x59 | Y
		b(00000000),
		b(00000000),
		b(10000010),
		b(10000010),
		b(01000100),
		b(01000100),
		b(00101000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00000000),
	},
	{   //   90 | 0x5A | Z
		b(00000000),
		b(00000000),
		b(01111110),
		b(00000010),
		b(00000010),
		b(00000100),
		b(00001000),
		b(00010000),
		b(00100000),
		b(01000000),
		b(01000000),
		b(01111110),
		b(00000000),
		b(00000000),
	},
	{   //   91 | 0x5B | [
		b(00000000),
		b(00111100),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00111100),
		b(00000000),
	},
	{   //   92 | 0x5C | \ (BACKSLASH)
		b(00000000),
		b(01000000),
		b(01000000),
		b(00100000),
		b(00100000),
		b(00010000),
		b(00010000),
		b(00001000),
		b(00001000),
		b(00000100),
		b(00000100),
		b(00000010),
		b(00000010),
		b(00000000),
	},
	{   //   93 | 0x5D | ]
		b(00000000),
		b(00111100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00111100),
		b(00000000),
	},
	{   //   94 | 0x5E | ^
		b(00000000),
		b(00010000),
		b(00101000),
		b(01000100),
		b(10000010),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   95 | 0x5F | _
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(11111111),
		b(00000000),
		b(00000000),
	},
	{   //   96 | 0x60 | `
		b(00000000),
		b(00010000),
		b(00010000),
		b(00001000),
		b(00001000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   97 | 0x61 | a
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00011100),
		b(00100010),
		b(00000010),
		b(00111110),
		b(01000010),
		b(01000110),
		b(00111010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   98 | 0x62 | b
		b(00000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01011100),
		b(01100010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //   99 | 0x63 | c
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  100 | 0x64 | d
		b(00000000),
		b(00000010),
		b(00000010),
		b(00000010),
		b(00111110),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000110),
		b(00111010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  101 | 0x65 | e
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(01111110),
		b(01000000),
		b(01000000),
		b(00111100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  102 | 0x66 | f
		b(00000000),
		b(00001110),
		b(00010000),
		b(00010000),
		b(01111100),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  103 | 0x67 | g
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00111010),
		b(01000110),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000110),
		b(00111010),
		b(00000010),
		b(01000010),
		b(00111100),
	},
	{   //  104 | 0x68 | h
		b(00000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01011100),
		b(01100010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  105 | 0x69 | i
		b(00000000),
		b(00000000),
		b(00010000),
		b(00000000),
		b(01110000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(01111100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  106 | 0x6A | j
		b(00000000),
		b(00000100),
		b(00000000),
		b(00111100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(00000100),
		b(01000100),
		b(00111000),
		b(00000000),
	},
	{   //  107 | 0x6B | k
		b(00000000),
		b(00000000),
		b(01000000),
		b(01000000),
		b(01000010),
		b(01000100),
		b(01001000),
		b(01110000),
		b(01001000),
		b(01000100),
		b(01000010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  108 | 0x6C | l
		b(00000000),
		b(01100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00100000),
		b(00011100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  109 | 0x6D | m
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(11101100),
		b(10010010),
		b(10010010),
		b(10010010),
		b(10010010),
		b(10010010),
		b(10010010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  110 | 0x6E | n
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01011100),
		b(01100010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  111 | 0x6F| o
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  112 | 0x70 | p
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01011100),
		b(01100010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01111100),
		b(01000000),
		b(01000000),
		b(01000000),
	},
	{   //  113 | 0x71 | q
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00111110),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00111010),
		b(00000010),
		b(00000010),
		b(00000010),
	},
	{   //  114 | 0x72 | r
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01011100),
		b(01100010),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(01000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  115 | 0x73 | s
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00111100),
		b(01000010),
		b(01000000),
		b(00111100),
		b(00000010),
		b(01000010),
		b(00111100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  116 | 0x74 | t
		b(00000000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(01111110),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00001110),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  117 | 0x75 | u
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000010),
		b(01000110),
		b(00111010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  118 | 0x76 | v
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(01000010),
		b(00100100),
		b(00100100),
		b(00100100),
		b(00011000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  119 | 0x77 | w
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(10000010),
		b(10000010),
		b(10010010),
		b(10010010),
		b(10101010),
		b(01000100),
		b(01000100),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  120 | 0x78 | x
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(00100100),
		b(00011000),
		b(00100100),
		b(01000010),
		b(01000010),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  121 | 0x79 | y
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01000010),
		b(01000010),
		b(00100010),
		b(00100100),
		b(00010100),
		b(00010100),
		b(00001000),
		b(00001000),
		b(00001000),
		b(00110000),
	},
	{   //  122 | 0x7A | z
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(01111110),
		b(00000010),
		b(00000100),
		b(00001000),
		b(00010000),
		b(00100000),
		b(01111110),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  123 | 0x78 | {
		b(00000000),
		b(00001100),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(01100000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00001100),
	},
	{   //  124 | 0x78 | |
		b(00000000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00000000),
	},
	{   //  125 | 0x7D | }
		b(00000000),
		b(01100000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00001100),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(00010000),
		b(01100000),
	},
	{   //  126 | 0x7E | ~
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00110010),
		b(01001100),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},
	{   //  127 | 0x7F | DEL
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
		b(00000000),
	},

};
