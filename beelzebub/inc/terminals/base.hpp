#pragma once

#include <terminals/interface.hpp>
#include <metaprogramming.h>
#include <handles.h>

namespace Beelzebub { namespace Terminals
{
	/**
	 * Represents a set of terminal capabilities.
	 **/
	class TerminalBase
	{
	public:

		/*  STATICS  */

		static const uint16_t DefaultTabulatorWidth = 8;

		/*  Writing  */

		static __bland TerminalWriteResult DefaultWriteCharAtXy(TerminalBase * const term, const char c, const int16_t x, const int16_t y);
		static __bland TerminalWriteResult DefaultWriteCharAtCoords(TerminalBase * const term, const char c, const TerminalCoordinates pos);
		static __bland TerminalWriteResult DefaultWriteStringAt(TerminalBase * const term, const char * const str, const TerminalCoordinates pos);

		static __bland TerminalWriteResult DefaultWriteChar(TerminalBase * const term, const char c);
		static __bland TerminalWriteResult DefaultWriteString(TerminalBase * const term, const char * const str);
		static __bland TerminalWriteResult DefaultWriteStringLine(TerminalBase * const term, const char * const str);

		/*  Positioning  */

		static __bland Result DefaultSetCursorPositionXy(TerminalBase * const term, const int16_t x, const int16_t y);
		static __bland Result DefaultSetCursorPositionCoords(TerminalBase * const term, const TerminalCoordinates pos);
		static __bland TerminalCoordinates DefaultGetCursorPosition(TerminalBase * const term);

		static __bland Result DefaultSetCurrentPositionXy(TerminalBase * const term, const int16_t x, const int16_t y);
		static __bland Result DefaultSetCurrentPositionCoords(TerminalBase * const term, const TerminalCoordinates pos);
		static __bland TerminalCoordinates DefaultGetCurrentPosition(TerminalBase * const term);

		static __bland Result DefaultSetSizeXy(TerminalBase * const term, const int16_t w, const int16_t h);
		static __bland Result DefaultSetSizeCoords(TerminalBase * const term, const TerminalCoordinates pos);
		static __bland TerminalCoordinates DefaultGetSize(TerminalBase * const term);

		static __bland Result DefaultSetBufferSizeXy(TerminalBase * const term, const int16_t w, const int16_t h);
		static __bland Result DefaultSetBufferSizeCoords(TerminalBase * const term, const TerminalCoordinates pos);
		static __bland TerminalCoordinates DefaultGetBufferSize(TerminalBase * const term);

		static __bland Result DefaultSetTabulatorWidth(TerminalBase * const term, const uint16_t w);
		static __bland uint16_t DefaultGetTabulatorWidth(TerminalBase * const term);

		/*  Styling  */

		/*	Utilitary methods  */

		/*static __bland TerminalWriteResult WriteIntD(const int64_t val);
		static __bland TerminalWriteResult WriteUintD(const uint64_t val);

		static __bland TerminalWriteResult WriteHex8(const uint8_t val);
		static __bland TerminalWriteResult WriteHex16(const uint16_t val);
		static __bland TerminalWriteResult WriteHex32(const uint32_t val);
		static __bland TerminalWriteResult WriteHex64(const uint64_t val);//*/
		
		/*  DYNAMICS  */

		/*  Descriptor  */

		const TerminalDescriptor * Descriptor;

		/*  Constructor  */

		__bland TerminalBase(const TerminalDescriptor * const desc);

		/*  Writing  */

		__bland TerminalWriteResult WriteAt(const char c, const int16_t x, const int16_t y);
		__bland TerminalWriteResult WriteAt(const char c, const TerminalCoordinates pos);
		__bland TerminalWriteResult WriteAt(const char * const str, const TerminalCoordinates pos);

		__bland TerminalWriteResult Write(const char c);
		__bland TerminalWriteResult Write(const char * const str);
		__bland TerminalWriteResult WriteLine(const char * const str);

		/*  Positioning  */

		__bland Result SetCursorPosition(const int16_t x, const int16_t y);
		__bland Result SetCursorPosition(const TerminalCoordinates pos);
		__bland TerminalCoordinates GetCursorPosition();

		__bland Result SetCurrentPosition(const int16_t x, const int16_t y);
		__bland Result SetCurrentPosition(const TerminalCoordinates pos);
		__bland TerminalCoordinates GetCurrentPosition();

		__bland Result SetSize(const int16_t w, const int16_t h);
		__bland Result SetSize(const TerminalCoordinates pos);
		__bland TerminalCoordinates GetSize();

		__bland Result SetBufferSize(const int16_t w, const int16_t h);
		__bland Result SetBufferSize(const TerminalCoordinates pos);
		__bland TerminalCoordinates GetBufferSize();

		__bland Result SetTabulatorWidth(const uint16_t w);
		__bland uint16_t GetTabulatorWidth();

	protected:

		TerminalCoordinates CurrentPosition;
		uint16_t TabulatorWidth;
	};
}}
