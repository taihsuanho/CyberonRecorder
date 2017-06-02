#define FRAME_SIZE        4     // Has to be power of 2 for fast mod(%) operation.
#define HALF_SIZE         (FRAME_SIZE >> 1)
#define HALF_BUF_SIZE     (HALF_SIZE * 3)

#define TICKS_8KHZ        1920  // Should be 16MHz/prescale/8kHz = 2000, but the tone sounds higher.
#define TICKS_11KHZ       1320
#define TICKS_16KHZ       880
#define TICKS_22KHZ       620

#define CMD_START         '1'   // Start recording
#define CMD_STOP          '2'   // Stop recording

#define MIC_PIN           A1
#define BAUD_RATE         921600

bool flagRecord = false, flag0 = false, flag1 = false;
int count = 0;
char buf[HALF_BUF_SIZE * 2];

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

  ADCSRA |= (1 << ADATE);             // enable auto trigger
  ADCSRA |= (1 << ADEN);              // enable ADC
  ADCSRA |= (1 << ADSC);              // start ADC measurements
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
  *b++ = p[0] ^ p[1] ^ 0xFF;          // Checksum byte: XOR of high byte, low byte, and 0xFF
  count++;                            // One more sample collected.
  flag0 = (count == HALF_SIZE);       // Set flag when the first buffer is full.
  flag1 = (count == FRAME_SIZE);      // Set flag when the second buffer is full.
  count = count & (FRAME_SIZE - 1);   // mod(%) operation when FRAME_SIZE is power of 2.
}

void setup()
{
  // Setup for recording 16kHz audio and transmission at baud 921600.
  Serial.begin(BAUD_RATE);
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  pinMode(MIC_PIN, INPUT);
  freeRunningModeADC(MIC_PIN);
  init_timer1_prescale1(TICKS_16KHZ, addSample);
}

void loop()
{
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
  while(Serial.available()){
    switch(Serial.read()){
    case CMD_START: startRecord(); break;
    case CMD_STOP:  stopRecord(); break;
    }
  }
}

void startRecord()
{
  if (!flagRecord){
    count = 0;
    flag0 = flag1 = false;
    flagRecord = true;
    digitalWrite(12, HIGH);
  }
}

void stopRecord()
{
  flagRecord = false; 
  digitalWrite(12, LOW); 
}


