#include <Wire.h>
#include <si5351.h>
#include <TimerOne.h>

#define ENC_DECODER (1 << 2)
#include <ClickEncoder.h>

#include <U8glib.h>

Si5351 si5351;


// sck, mosi, cs, a0, reset
U8GLIB_SSD1306_128X64 u8g(13, 12, 0, 11, 10); // CS is not used
ClickEncoder *encoder;

int16_t last, value;
long long startingFrequency = 110000000; // 110MHz in Hz
long long f = startingFrequency; // in Hz
long long stepSize = 1;
unsigned char stepIndex = 1;

void timerIsr() {
	encoder->service();
}

void setup() {
	Serial.begin(9600);
	encoder = new ClickEncoder(A1, A0, A2);

	Timer1.initialize(1000);
	Timer1.attachInterrupt(timerIsr); 

	last = value = 0;
	stepIndex = 3;

	si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);
	si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
	si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);  

	setFrequency(f);
	renderFrequency(f);
	renderStepSize();
	render();
}


void loop() {
	// only add to frequency when in normal screen
	value += encoder->getValue();

	ClickEncoder::Button b = encoder->getButton();
	if (b != ClickEncoder::Open) {
		switch (b) {
			case ClickEncoder::Clicked:
				last = value = 0;
				stepIndex++;
				stepIndex %= 6;

        stepSize = 1;
        for (int i = 0; i < stepIndex; i++)
          stepSize *= 10;
       
				renderStepSize();
				break;
			case ClickEncoder::DoubleClicked:
				encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
				Serial.print("Acceleration is ");
				Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
				break;
		}
	}
    
	if (value != last) {
		last = value;

		int amountToAdd = (value / 3);
		if (amountToAdd != 0) {
			f += amountToAdd * stepSize;
			value -= amountToAdd * 3;
		}
		renderFrequency(f);
		setFrequency(f);
	}
    render();
}

void setFrequency(long long frequency) {
	si5351.set_freq(frequency * 100LL, 0ULL, SI5351_CLK0);
}

char stepSizeBuffer[20];
void renderStepSize() {
	switch (stepIndex) {
    case 0: sprintf(stepSizeBuffer, "Step: 1Hz", stepSize); break;
		case 1: sprintf(stepSizeBuffer, "Step: 10Hz", stepSize); break;
		case 2: sprintf(stepSizeBuffer, "Step: 100Hz", stepSize); break;
		case 3: sprintf(stepSizeBuffer, "Step: 1KHz", stepSize); break;
		case 4: sprintf(stepSizeBuffer, "Step: 10KHz", stepSize); break;
		case 5: sprintf(stepSizeBuffer, "Step: 100KHz", stepSize); break;
		default: break;
	}
}

char frequencyRepr[15] = {' ', '2', '8', '.', '1', '1', '0', '.', '2', '0', ' ', 'M', 'H', 'z', '\0'};
void renderFrequency(long long f) {
	frequencyRepr[2] = f >= 1000000 ? '.' : ' ';

	int unit = 0;
	bool hasHundreds = false;

	unit = f / 100000000LL;
	if (unit > 0) {
		frequencyRepr[0] = '0' + unit;
		hasHundreds = true;
	} else {
		frequencyRepr[0] = ' ';
	}

	unit = (f / 10000000LL) % 10;
	if (unit > 0 || hasHundreds) {
		frequencyRepr[1] = '0' + unit;
	} else {
		frequencyRepr[1] = ' ';
	}

	unit = (f / 1000000LL) % 10;
	frequencyRepr[2] = '0' + unit;

	unit = (f / 100000LL) % 10;
	frequencyRepr[4] = '0' + unit;

	unit = (f / 10000LL) % 10;
	frequencyRepr[5] = '0' + unit;

	unit = (f / 1000LL) % 10;
	frequencyRepr[6] = '0' + unit;

	unit = (f / 100LL) % 10;
	frequencyRepr[8] = '0' + unit;

	unit = (f / 10LL) % 10;
	frequencyRepr[9] = '0' + unit;
}


void render() {
	u8g.firstPage();  
	do {
		draw();
	} while(u8g.nextPage());
}


void draw(void) {
	u8g.setFont(u8g_font_tpss);
  u8g.drawStr(30, 12, frequencyRepr);
	u8g.drawStr(40, 30, stepSizeBuffer);
}


