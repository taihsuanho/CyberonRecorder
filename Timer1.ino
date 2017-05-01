
int gStartTicks;
void (*gTimer1_func)(void);

void init_timer1_prescale1(int ticks, void (*func_ptr)(void))
{
  TCCR1A = 0x00;                  // Normal mode, just as a Timer
  TCCR1B = 0x00;
  TCCR1B &= ~(1 << CS12);         // prescaler = CPU clock/1
  TCCR1B &= ~(1 << CS11);       
  TCCR1B |=  (1 << CS10);    
  TIMSK1 |=  (1 << TOIE1);        // enable timer overflow interrupt
  TCNT1 = gStartTicks = 65535 - ticks;  
  gTimer1_func = func_ptr;
}

ISR(TIMER1_OVF_vect)
{
  (*gTimer1_func)();
  TCNT1 = gStartTicks;
}

