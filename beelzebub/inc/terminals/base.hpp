#pragma once

#include <metaprogramming.h>
#include <handles.h>

namespace Beelzebub { namespace Terminals
{
	/**
	 * Known major terminal types.
	 **/
	enum class TerminalType : u8
	{
		Unknown     = 0x00,

		TextMatrix  = 0x01,
		PixelMatrix = 0x02,

		Serial      = 0x10,

		Keyboard    = 0xFF,
	};

	/**
	 * Represents a set of terminal capabilities.
	 **/
	struct TerminalCapabilities
	{
		bool CanOutput;            //  Characters can be written to the terminal.
		bool CanInput;             //  Characters can be received from the terminal.
		bool CanRead;              //  Characters can be read back from the terminal's output.

		bool CanGetOutputPosition; //  Position of next output character can be retrieved.
		bool CanSetOutputPosition; //  Position of output characters can be set arbitrarily.
		bool CanPositionCursor;    //  Terminal features a positionable cursor.

		bool CanGetSize;           //  Terminal (window) size can be retrieved.
		bool CanSetSize;           //  Terminal (window) size can be changed.

		bool Buffered;             //  Terminal acts as a window over a buffer.
		bool CanGetBufferSize;     //  Buffer size can be retrieved.
		bool CanSetBufferSize;     //  Buffer size can be changed.
		bool CanPositionWindow;    //  The "window" can be positioned arbitrarily over the buffer.

		bool CanColorBackground;   //  Area behind/around output characters can be colored.
		bool CanColorForeground;   //  Output characters can be colored.
		bool FullColor;            //  32-bit BGRA, or ARGB in little endian.
		bool ForegroundAlpha;      //  Alpha channel of foreground color is supported. (ignored if false)
		bool BackgroundAlpha;      //  Alpha channel of background color is supported. (ignored if false)

		bool CanBold;              //  Output characters can be made bold.
		bool CanUnderline;         //  Output characters can be underlined.
		bool CanBlink;             //  Output characters can blink.

		bool CanGetStyle;          //  Current style settings can be retrieved.

		bool CanGetTabulatorWidth; //  Tabulator width may be retrieved.
		bool CanSetTabulatorWidth; //  Tabulator width may be changed.
		
		bool SequentialOutput;     //  Character sequences can be output without explicit position.

		bool SupportsTitle;        //  Supports assignment of a title.

		TerminalType Type;         //  The known type of the terminal.
	};

	/**
	 * Represents a set of coordinates within a terminal's matrix.
	 * They can represent a position or a size.
	 **/
	struct TerminalCoordinates
	{
		int16_t X;
		int16_t Y;

		__bland TerminalCoordinates operator+(const TerminalCoordinates other);
		__bland TerminalCoordinates operator-(const TerminalCoordinates other);
	};

	const TerminalCoordinates InvalidCoordinates = { -42, 9001 };

	/**
	 * Represents the result of a write operation.
	 **/
	struct TerminalWriteResult
	{
		Beelzebub::Result Result;
		uint32_t Size;
		TerminalCoordinates End;
	};

	/**
	 * Represents a set of terminal capabilities.
	 **/
	class TerminalBase
	{
	public:
		/*  Statics  */

		static const uint16_t DefaultTabulatorWidth = 8;

		/*  Capabilities  */

		TerminalCapabilities Capabilities;

		/*  Writing  */

		virtual __bland TerminalWriteResult WriteAt(const char c, const int16_t x, const int16_t y) = 0;
		virtual __bland TerminalWriteResult WriteAt(const char c, const TerminalCoordinates pos);
		virtual __bland TerminalWriteResult WriteAt(const char * const str, const TerminalCoordinates pos);

		virtual __bland TerminalWriteResult Write(const char c);
		virtual __bland TerminalWriteResult Write(const char * const str);
		virtual __bland TerminalWriteResult WriteLine(const char * const str);

		/*  Positioning  */

		virtual __bland Result SetCursorPosition(const int16_t x, const int16_t y) = 0;
		virtual __bland Result SetCursorPosition(const TerminalCoordinates pos);
		virtual __bland TerminalCoordinates GetCursorPosition() = 0;

		virtual __bland Result SetCurrentPosition(const int16_t x, const int16_t y);
		virtual __bland Result SetCurrentPosition(const TerminalCoordinates pos);
		virtual __bland TerminalCoordinates GetCurrentPosition();

		virtual __bland Result SetSize(const int16_t w, const int16_t h) = 0;
		virtual __bland Result SetSize(const TerminalCoordinates pos);
		virtual __bland TerminalCoordinates GetSize() = 0;

		virtual __bland Result SetBufferSize(const int16_t w, const int16_t h) = 0;
		virtual __bland Result SetBufferSize(const TerminalCoordinates pos);
		virtual __bland TerminalCoordinates GetBufferSize() = 0;

		virtual __bland Result SetTabulatorWidth(const uint16_t w);
		virtual __bland uint16_t GetTabulatorWidth();

		/*  Styling  */

		/*	Utilitary methods  */

		/*virtual __bland TerminalWriteResult WriteIntD(const int64_t val);
		virtual __bland TerminalWriteResult WriteUintD(const uint64_t val);

		virtual __bland TerminalWriteResult WriteHex8(const uint8_t val);
		virtual __bland TerminalWriteResult WriteHex16(const uint16_t val);
		virtual __bland TerminalWriteResult WriteHex32(const uint32_t val);
		virtual __bland TerminalWriteResult WriteHex64(const uint64_t val);
//*/
	protected:
		TerminalCoordinates CurrentPosition;
		uint16_t TabulatorWidth;
	};
}}
