  
#define BAUD_RATE         921600
#define FRAME_SIZE        8    // Has to be power of 2 for fast mod(%) operation.
#define HALF_SIZE         (FRAME_SIZE >> 1)
#define HALF_BUF_SIZE     (HALF_SIZE * 3)

#define TICKS_8KHZ        1920  // Should be 16MHz/prescale/8kHz = 2000, but the tone sounds higher.
#define TICKS_11KHZ       1320
#define TICKS_16KHZ       880
#define DC_OFFSET         0

#define CMD_START         0x31  // Start recording
#define CMD_STOP          0x32  // Stop recording
#define CMD_8KHZ          0x41  // Set 8kHz sampling rate
#define CMD_11KHZ         0x42  // Set 11kHz sampling rate
#define CMD_16KHZ         0x43  // Set 16kHz sampling rate

#define MIC_PIN           A1

bool flagRecord = false, flag0 = false, flag1 = false;
int count = 0;
char buf[FRAME_SIZE * 3];  // for 16-bit sample plus 8-bit checksum

void freeRunningModeADC(int micPin)
{
  ADCSRA = 0;                         // clear ADCSRA register
  ADCSRB = 0;                         // clear ADCSRB register
  ADMUX |= ((micPin - A0) & 0x07);    // set analog input pin
  ADMUX |= (1 << REFS0);              // set reference voltage
  ADMUX |= (1 << ADLAR);              // left align ADC

  // sampling rate is [ADC clock] / [prescaler] / [conversion clock cycles]
  // for Arduino Uno ADC clock is 16 MHz and a conversion takes 13 clock cycles
  ADCSRA |= (1 << ADPS2);             // 16 prescaler for 76.9 KHz

  ADCSRA |= (1 << ADATE); // enable auto trigger
  ADCSRA |= (1 << ADEN);  // enable ADC
  ADCSRA |= (1 << ADSC);  // start ADC measurements
}

void addSample()
{
  if (!flagRecord)
    return;

  // Add incoming sample to the ping-pong buffers.
  short sample = ADCW - 32767; 
  char *p = (char *)&sample;
  char *b = &buf[count * 3];
  *b++ = p[0];                        // Low byte of the sample.
  *b++ = p[1];                        // High byte of the sample.
  *b++ = p[0] ^ p[1] ^ 0xFF;          // Checksum.
  count++;                            // One more sample collected.
  flag0 = (count == HALF_SIZE);       // Set flag when the first buffer is full.
  flag1 = (count == FRAME_SIZE);      // Set flag when the second buffer is full.
  count = count & (FRAME_SIZE - 1);   // mod(%) operation when FRAME_SIZE is power of 2.
}

void setup()
{
  Serial.begin(BAUD_RATE);
  pinMode(MIC_PIN, INPUT);
  freeRunningModeADC(MIC_PIN);
  init_timer1_prescale1(TICKS_16KHZ, addSample);
}

void loop()
{
  int i;

  // Return immediately if it is not recording.
  if (!flagRecord)
    return;
  
  // Write data in ping-pong buffers to serial port when they are full, and then reset the flags.
  if (flag0) {
    flag0 = false;
    Serial.write(buf, HALF_BUF_SIZE);
  }
  if (flag1) {
    flag1 = false;
    Serial.write(buf + HALF_BUF_SIZE, HALF_BUF_SIZE);
  }
}

void serialEvent()
{
  char c = Serial.read();

  if (!flagRecord && c == CMD_START){
      count = 0;
      flag0 = flag1 = false;
      flagRecord = true;
  }
  else if (flagRecord && c == CMD_STOP) {
    flagRecord = false;
  }
  else if (c == CMD_8KHZ){
    init_timer1_prescale1(TICKS_8KHZ, addSample);
  }
  else if (c == CMD_11KHZ){
    init_timer1_prescale1(TICKS_11KHZ, addSample);
  }
  else if (c == CMD_16KHZ){
    init_timer1_prescale1(TICKS_16KHZ, addSample);
  }
}

