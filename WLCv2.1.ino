///////////////////////////////////////////////////////////////////////////
/*
 * pull up all the sensor pins with 4.7k ohm 
 *                  wired                   --> low, top, sump, dry run 
 *     Or           wireless (Receiver)     --> sump
 *                  wireless (Transmitter)  --> Low, Top, Dry run
 *                  
 *                  For wireless Type only, Connect Receive_Valid_Transmission LED possitive of the HT12D Receiver module  to pin A5
 *                  
 *                  
 * For LEDs use 560ohm on postive side
 * 
 */
///////////////////////////////////////////////////////////////////////////

//It Supports both Wired and Wireless Technique

#define Type 0                         // 0 -> Wired,     1 -> Wireless
#define Phase 1                         // 1 -> Single Phase,     3 -> Three Phase
#define Float 0                         // 0 -> Small Float,     1 -> Big Float

#define Sump 2                           // 1 -> Single Float, 2 -> Dual Small Float;    ( Dual small float : this is for small sump tank which oscillates highly. So to get sump sensor value steadily without oscilation we use dual sensor. Below Low Sensor means sump sesnse is LOW; above top sensor means sump sense is HIGH )

#define Dry_Run                         // Use this macro, If You need Dry run Feature... Otherwise comment it
    #ifdef Dry_Run
      #define dry_run_delay 50              // How much time it will take to reach water to OHT tank ( Delay (sec) )
      #define dry_run_recover_delay 1       // When the unit should recover from Dry run Lock to Normal Working Condition ( in Minutes )
    #endif

#define itrn 5                          // Number of iterations for providing high sensitivity to the electrodes


//#define Motor_Protection                         // Use this macro, If You need Motor_Protection Feature...( i.e. Can set time period for motor On time and Off time ) Otherwise comment it
    #ifdef Motor_Protection
      #define motor_protection_MotorOff_time 30                      // How many minutes the motor should run. After completing the time, Motor will be Turned Off. units in minutes
      #define motor_protection_break_for_recovery 60       // How many minutes the break should take for motor. After completing the break, Motor will be Turned On. units in minutes
    #endif

#define motor_protection_on_time 0                 
    
///////////////////////////////////////////////////////////////////////////

# define low A0
# define top A1
# define dry A2
# define sump A3
# define sump2 A4
# define Receiver_Valid_Transmission A5

# define led_motor 2
# define led_low 3
# define led_top 4
# define led_dry 5
# define led_sump 6
# define manual_start 7



#if(Phase == 3)
  # define motor_start 10
  # define motor_stop 11
#else
  # define motor 10
#endif


/////////////////////////////////////////////////////////////////////////


bool motor_prev_state = 0;
bool motor_cmd = 0;
unsigned long dry_run_recover_prev_millis = 0;
unsigned long previousMillis = 0;
unsigned long motor_protect_turn_off_millis = 0;
unsigned long motor_protect_break_for_recovery_millis = 0;
bool dry_run_lock = 0;
bool dry_run_enable = 0;

bool permenent_motor_state = 0;
bool temp1 = 0;
bool temp2 = 0;

int temp3 = 0;

bool sump_status = 0;

void setup() 
{
  Serial.begin(9600);
  Serial.println("Started WLC 2.1");
  pinMode(low, INPUT_PULLUP);
  pinMode(top, INPUT_PULLUP);
  pinMode(manual_start, INPUT_PULLUP);

  pinMode(led_motor, OUTPUT);
  pinMode(led_low, OUTPUT);
  pinMode(led_top, OUTPUT);

  digitalWrite(led_motor, LOW);
  digitalWrite(led_low, LOW);
  digitalWrite(led_top, LOW);

  Serial.println("Initial Pin Configuration Finished");
  Serial.println();
  mode_confg();
//  pinMode(interruptPin, INPUT_PULLUP);
//  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);
}

void loop() 
{
  #if (Type == 1)
    if(analogRead(Receiver_Valid_Transmission)>=100)
    {
      sense();
      temp3 = 0;
    }
    else
    {
      delay(1000);
      temp3++;
      Serial.print("Disconnected count ( out of 10 ) : ");
      Serial.println(temp3);
    }
    if(temp3 > 10)
      Motor_Stop();
      
  #else
    sense();
  #endif
  
  #ifdef Motor_Protection
      if(permenent_motor_state)
      {
        if(motor_prev_state)
        {
          if (((millis()- motor_protect_turn_off_millis)/1000) >= motor_protection_MotorOff_time*60)
          {
            Serial.println("--------------------------  Attention  ------------------------------");
            Serial.print("MOTOR STOPED Due to Restriction for ");
            Serial.print(motor_protection_MotorOff_time);
            Serial.println(" Minutes Only");
            Serial.println("*Comment motor_protection for continues running");
            Serial.println("---------------------------------------------------------------------");
            Motor_ctrl(0);
            motor_protect_break_for_recovery_millis = millis();
          }
        }
        else if(!motor_prev_state)
        {
          if (((millis()- motor_protect_break_for_recovery_millis)/1000) >= motor_protection_break_for_recovery*60)
          {
            Serial.println("--------------------------  Attention  ------------------------------");
            Serial.print("MOTOR STARTED Due to recovery Delay of ");
            Serial.print(motor_protection_break_for_recovery);
            Serial.println(" Minutes Only");
            Serial.println("*Comment motor_protection for continues running");
            Serial.println("---------------------------------------------------------------------");
            Motor_ctrl(1);
            motor_protect_turn_off_millis = millis();
          }
        }
      }
  #endif


}

bool sump_sense()
{
    #if(Sump == 1)
      int sump_status = !get_input(sump);
      digitalWrite(led_sump, sump_status);
      return sump_status;
    #else if(Sump == 2)
      int sump_low = !get_input(sump);
      int sump_top = !get_input(sump2);
      if(!sump_low)
        sump_status = 0;
      else if(sump_low && sump_top)
        sump_status = 1;
      digitalWrite(led_sump, sump_status);
      return sump_status;
    #endif
}

void sense()
{
  
  bool l_status = get_input(low);
  bool t_status = get_input(top);
  digitalWrite(led_low, !l_status);
  digitalWrite(led_top, !t_status);

  #ifdef Sump
    if(sump_sense())
    {
      if(!temp1)
      {
        Serial.println("Sump Activated");
        temp1 = 1;
      }
      if(((l_status && !Float) || (!l_status && Float)) && t_status && !permenent_motor_state )
      {
        Motor_ctrl(1);
        permenent_motor_state = 1;
        motor_protect_break_for_recovery_millis = millis();
        motor_protect_turn_off_millis = millis();
      }
      else if(((!l_status && !Float) || (l_status && Float)) && !t_status )
      {
        Motor_ctrl(0);
        permenent_motor_state = 0;
      }
    }
    else
    {
      if(temp1)
      {
        Serial.println("Sump Deactivated");
        temp1 = 0;
      }
      Motor_ctrl(0);
      permenent_motor_state = 0;
    }
  #else
    if(((l_status && !Float) || (!l_status && Float)) && t_status && !permenent_motor_state)
    {
        Motor_ctrl(1);
        permenent_motor_state = 1;
        motor_protect_break_for_recovery_millis = millis();
        motor_protect_turn_off_millis = millis();
    }
    else if(((!l_status && !Float) || (l_status && Float)) && !t_status )
    {
        Motor_ctrl(0);
        permenent_motor_state = 0;
    }
  #endif

  #ifdef Dry_Run
    if ((((millis() - previousMillis)/1000) >= (dry_run_delay)) && dry_run_enable && motor_prev_state)
    {
      Serial.println("Dry Run Checking is in progress");
  
      if(get_input(dry))
      {
        digitalWrite(led_dry, get_input(dry));
        Serial.println("Motor will be Stoped dueto Dry Run Condition");
        Motor_ctrl(0);
        dry_run_lock = 1;
        dry_run_enable = 0;
        dry_run_recover_prev_millis = millis();
      }
      previousMillis = millis();
    }
  
//  Serial.print("Time : ");
//  Serial.print((millis() - dry_run_recover_prev_millis) / 1000);
//  Serial.print(" >= ");
//  Serial.println(dry_run_recover_delay*60);
//  Serial.print("dry_run_enable : ");
//  Serial.println(dry_run_enable);
//  Serial.print("motor_prev_state : ");
//  Serial.println(motor_prev_state);
//  Serial.print("dry_run_lock : ");
//  Serial.println(dry_run_lock);
  
  
  if((((millis() - dry_run_recover_prev_millis)/1000) >= dry_run_recover_delay*60) && !dry_run_enable && dry_run_lock )
  {
    dry_run_lock = 0;
    dry_run_enable = 0;
    Serial.println("Unit Returned from Dry run to Normal Mode");
    digitalWrite(led_dry, LOW);
  }

  #endif
  
//Serial.println(dry_run_lock);
  
//Serial.print("LOW : ");
//Serial.print(get_input(low));
//Serial.print("    Top : ");
//Serial.print(get_input(top));
//Serial.print("    SUMP : ");
//Serial.print(get_input(sump));
//Serial.print("    DRY : ");
//Serial.println(get_input(dry));

}


void Motor_ctrl(bool cmd)
{
//  Serial.println(dry_run_lock);
  if(!dry_run_lock)
  {
    if((cmd && !motor_prev_state))
      Motor_Start();
    else if(!cmd && motor_prev_state)
      Motor_Stop();
  }
}


void Motor_Start()
{
  
  #ifdef Dry_Run
    dry_run_enable = 1;
    previousMillis = millis();
  #endif
  
  digitalWrite(led_motor,HIGH);
  #if (Phase == 3)
    digitalWrite(motor_start,HIGH);
    delay(1500);
    digitalWrite(motor_start,LOW);
  #else
    digitalWrite(motor,HIGH);
  #endif
  
//  drycount=0;
  motor_prev_state = 1;
  Serial.println("Motor_Started");
}

void Motor_Stop()
{
  digitalWrite(led_motor,LOW);
  #if (Phase == 3)
    digitalWrite(motor_stop,HIGH);
    delay(1500);
    digitalWrite(motor_stop,LOW);
  #else
    digitalWrite(motor,LOW);
  #endif

//  drycount=0;
  motor_prev_state = 0;
  Serial.println("Motor_Stoped");
}

bool get_input(int pin)
{
  int temp = 0;
  bool tstatus = 0;
  for(int i=0; i<itrn; i++)
    temp += analogRead(pin);
  temp = temp/itrn ;
  tstatus = (temp > 400)?1:0;
  return(tstatus);
}






void mode_confg()
{
  Serial.println("-------------------------------------------");
  Serial.println();

  Serial.print("  TRx Type              : ");
  #if (Type == 1)
    Serial.println("Wireless Type");
    pinMode(Receiver_Valid_Transmission, INPUT);
  #else
    Serial.println("Wired Type");
  #endif


  
  Serial.print("  Phase                 : ");
  #if (Phase == 3)
    Serial.println("Three Phase");
    pinMode(motor_start, OUTPUT);
    pinMode(motor_stop, OUTPUT);
    digitalWrite(motor_start, LOW);
    digitalWrite(motor_stop, LOW);
  #else
    Serial.println("Single Phase");
    pinMode(motor, OUTPUT);
    digitalWrite(motor, LOW);
  #endif
  
  
  Serial.print("  Float                 : ");
  #if (Float == 0)
    Serial.println("Small Float");
  #else
    Serial.println("Big Float");
  #endif
  
  Serial.print("  Dry run               : ");
  #ifdef Dry_Run
    Serial.println("Included");
    pinMode(dry, INPUT_PULLUP);
    pinMode(led_dry, OUTPUT);
    digitalWrite(led_dry, LOW);
  #else
    Serial.println("Not Included");
  #endif
  
  Serial.print("  Sump                  : ");
  #ifdef Sump
    #if(Sump == 1)
      Serial.println("Included ( Single Float )");
      pinMode(sump, INPUT_PULLUP);
      pinMode(led_sump, OUTPUT);
      digitalWrite(led_sump, LOW);
    #else if(Sump == 2)
      Serial.println("Included ( Dual Float )");
      pinMode(sump, INPUT_PULLUP);
      pinMode(sump2, INPUT_PULLUP);
      pinMode(led_sump, OUTPUT);
      digitalWrite(led_sump, LOW);
    #endif
  #else
    Serial.println("Not Included");
  #endif
  
  Serial.print("  Motor_Protection      : ");
  #ifdef Motor_Protection
    Serial.println("Included");
  #else
    Serial.println("Not Included");
  #endif
  
  Serial.println();
  Serial.println("-------------------------------------------");
  Serial.println("Final Pin Configuration Finished");

}
