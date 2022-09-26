#ifndef FOUR_DIGITS_DISPLAY_H
#define FOUR_DIGITS_DISPLAY_H

#define DIGIT_DISPLAY_TIME_MS 1

#include <stdio.h>
#include <array>

class FourDigitsDisplay {
	public:
		FourDigitsDisplay(const std::array<uint, 4>& digitSelectPins, const std::array<uint, 7>& segmentPins);
		void init();
		void clear();
		void displayNumber(uint number);
		void refresh();
		
	private:
		std::array<uint, 4> digitSelectPins;
		std::array<uint, 7> segmentPins;

		std::array<uint, 4> currentNumber;
		uint digitCurrentlyDisplayed;
		bool digitDisplayed;
		uint64_t nextDigitDisplayTime;
		
		void displayDigit(uint digitSelect, uint digitToDisplay);

		inline constexpr static char SEGMENTS_CHARS[10][7] = {
				{0,0,0,0,0,0,1},
				{1,0,0,1,1,1,1},
				{0,0,1,0,0,1,0},
				{0,0,0,0,1,1,0},
				{1,0,0,1,1,0,0},
				{0,1,0,0,1,0,0},
				{0,1,0,0,0,0,0},
				{0,0,0,1,1,0,1},
				{0,0,0,0,0,0,0},
				{0,0,0,0,1,0,0},
		};

};

#endif
