#include <stdio.h> 

typedef enum class ButtonEvent {
	PUSHED, RELEASED, NONE
} ButtonEvent;

class PushButton {
	public:
		PushButton(uint pin);
		void init();
		ButtonEvent pollEvent();
		
	private:
		const uint pin;
		bool lastPinState;
		uint64_t debounceEndTime;
};

