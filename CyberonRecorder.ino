#define FRAME_SIZE        4     // Has to be power of 2 for fast mod(%) operation.
#define HALF_SIZE         (FRAME_SIZE >> 1)
#define HALF_BUF_SIZE     (HALF_SIZE * 3)

#define TICKS_8KHZ        1920  // Should be 16MHz/prescale/8kHz = 2000, but the tone sounds higher.
#define TICKS_11KHZ       1320
#define TICKS_16KHZ       880
#define TICKS_22KHZ       620

#define CMD_ACK           '0'   // ACK simply return 0x00
#define CMD_START         '1'   // Start recording
#define CMD_STOP          '2'   // Stop recording

#define CMD_8KHZ          'A'   // Set 8kHz sampling rate
#define CMD_11KHZ         'B'   // Set 11kHz  
#define CMD_16KHZ         'C'   // Set 16kHz  
#define CMD_22KHZ         'D'   // Set 22kHz

#define CMD_CH1           'a'   // Set mono channel

#define CMD_BAUD115200    'o'   // Set baud 115200
#define CMD_BAUD460800    'p'   // Set baud 460800
#define CMD_BAUD921600    'q'   // Set baud 921600
#define CMD_BAUD1000000   'r'   // Set baud 1M

#define CMD_VERSION       'V'   // Get version (optional for recorder)
#define CMD_CAPABILITY    'X'   // Get capability (optional for recorder)

#define MIC_PIN           A1

// JSON format strings for description of the recording device. (optional for recorder)
const char *strVersion = "\r\n{\"hardware\":\"Arduino UNO R3\", \"firmware\":\"CyberVoice\", \"version\":\"1.0\"}\r\n";
const char *strCapability = "\r\n{\"capability\":\"012ABCDaopqrVX\"}\r\n";
                      
bool flagRecord = false, flag0 = false, flag1 = false;
int count = 0;
char buf[HALF_BUF_SIZE * 2];
long baud = 0;

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
  Serial.begin(115200);               // Initial baud 115200 to comply with CyberVoice command protocol.
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  pinMode(MIC_PIN, INPUT);
  freeRunningModeADC(MIC_PIN);
  init_timer1_prescale1(TICKS_16KHZ, addSample);
}

void loop()
{
  int i;

  // Change baud rate if any CMD_BAUD* event was received. Remember to flush before changing baud rate.
  if (baud != 0){
    Serial.flush();     
    Serial.begin(baud);
    baud = 0;
  }
  
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
  static char cmdBuf[4] = {0, 0, 0, 0};

  while(Serial.available()){
    cmdBuf[3] = Serial.read();
    if (strncmp(cmdBuf, "CYB", 3) == 0){
      switch(cmdBuf[3]){
      case CMD_ACK: Serial.write(0); break;
      case CMD_VERSION: Serial.write(strVersion, strlen(strVersion)); break;
      case CMD_CAPABILITY: Serial.write(strCapability, strlen(strCapability)); break;
      case CMD_START: startRecord(); break;
      case CMD_STOP:  stopRecord(); break;
      case CMD_8KHZ:  init_timer1_prescale1(TICKS_8KHZ, addSample);  Serial.write(0); break;
      case CMD_11KHZ: init_timer1_prescale1(TICKS_11KHZ, addSample); Serial.write(0); break;
      case CMD_16KHZ: init_timer1_prescale1(TICKS_16KHZ, addSample); Serial.write(0); break;
      case CMD_22KHZ: init_timer1_prescale1(TICKS_22KHZ, addSample); Serial.write(0); break;
      case CMD_CH1: Serial.write(0); break;  
      case CMD_BAUD115200:  baud = 115200;  Serial.write(0); break;
      case CMD_BAUD460800:  baud = 460800;  Serial.write(0); break;
      case CMD_BAUD921600:  baud = 921600;  Serial.write(0); break;
      case CMD_BAUD1000000: baud = 1000000; Serial.write(0); break;
      default: Serial.write(0xff);  // return 0xff for unsupported commands.
      }
    }
    memmove(cmdBuf, cmdBuf + 1, 3); 
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


