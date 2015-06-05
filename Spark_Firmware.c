//Include necessary libraries
#include "Serial2/Serial2.h" 
#include "elapsedMillis/elapsedMillis.h"

elapsedMillis elapsedTime;

//Global variables
int id = 4242;
int count1 = 0;
int count2 = 0;
int litres1 = 0;
int litres2 = 0;
int temperature1 = 0;
int temperature2 = 0;
int temperature3 = 0;
int temperature4 = 0;
int PMIC = 0;
byte commsLoopCounter = 0;
byte commsFailCounter = 0;
byte modemResetCounter = 0;
byte controlMode = 0;    // 0 is OFF. 1 is AUTO. 2 is ON.

//Flags
byte drip = 0;
String valveState = "CLOSED";
String relayState = "OFF";
byte incLitres1 = 0;
byte incLitres2 = 0;
boolean modemConnected = false;
boolean modemReset = false;
byte relay = 0;

//Declare methods
byte updateDripsensor();
void valveSet(byte set);
void incrementCount1();
void incrementCount2();

//Setup
//------------------------------------------------------------------------------------------------
void setup() 
{
  //Define local variables
  //Define variables to hold colour values
  byte Red = 255;
  byte Green = 0;
  byte Blue = 0;
  byte temp = 0;
  
  //Define debug timout counter
  byte debugCount = 0;
  
  //Initialize serial connections
  Serial.begin(9600);
  Serial1.begin(600);
  Serial2.begin(115200);
  
  //Debug initialization
  //--------------------------------------------------------------------------------------------------
  //Spark will wait 10 seconds for debug interface to be opened
  //RGB will blink indicating that this state is active.
  //Background tasks will still be run so the Spark is receptive to hardware updates
  //If debug is enabled debug messages will be printed to the serial terminal preceeded by DEBUG: 
  
  //Take control of RGB LED
  RGB.control(true);
  
  while(!Serial.available() && ++debugCount < 40)
  {
    SPARK_WLAN_Loop();
    RGB.color(Red, Green, Blue);
    
    //Change colour of RGB LED
    temp = Red;
    Red = Green;
    Green = Blue;
    Blue = temp;
    
    //Delay half a second
    delay(500);
  }
  
  //Return control of RGB to Particle
  RGB.control(false);
  //---------------------------------------------------------------------------------------------------
  
  //Begin initialization of pinmodes 
  Serial.println("DEBUG: Initializing");

  //Define pinmodes
  Serial.println("DEBUG: Defining pinmodes");
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
  pinMode(A7, OUTPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  
  //Set initial states
  Serial.println("DEBUG: Setting initial states");
  digitalWrite(D4, LOW);
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(A7, HIGH);
  
  //Attach interrupts
  Serial.println("DEBUG: Attaching interrupts");
  attachInterrupt(D2, setRelayFlag, CHANGE);
  attachInterrupt(A5, incrementCount1, CHANGE);
  attachInterrupt(A6, incrementCount2, CHANGE);
  
  //Set up PMIC
  Serial.println("DEBUG: Initializing PMIC");
  
  //Set current gain to 10x
  Serial1.write(0x80);	//Select page 0
  Serial1.write(0x40);	//Select address zero
  Serial1.write(0x00);
  Serial1.write(0x20);
  Serial1.write(0xC0);
		
  //Enable high pass filter
  Serial1.write(0x90); //Select page 16
  Serial1.write(0x40); //Select address 0
  Serial1.write(0x0A); 
  Serial1.write(0x02); 
  Serial1.write(0x10); 
	
  Serial.println("DEBUG: Initialization complete");	
	
  //Start continuous conversion
  Serial1.write(0xD5); 

  //Set initial state of valve to closed
  valveSet(0);
  //Set initial state of relay to off
  relaySet(0);
 
 //Test SMS
 
 Serial2.println("AT+CMGF=1");
  while(Serial2.available())
 {
     Serial2.read();
 }
 Serial2.println("AT+CMGS=\"+27715812181\"");
 while(Serial2.available())
 {
     Serial2.read();
 }
 Serial2.println("Hello. From a friendly modem near you.\x1A");
  
}
//------------------------------------------------------------------------------------------------

//Main program loop
//------------------------------------------------------------------------------------------------
void loop() 
{
  //Loops continuously. Flag checking and burst geyser detection        
  continuous();
  
  //5 Second control loop
  if (elapsedTime > 5000)
    control();
}
//------------------------------------------------------------------------------------------------
//Control loop that runs continuously
void continuous()
{
  //Check for burst geyser only if current drip status is OK
  if (drip == 0)
  {
    //drip = updateDripsensor(); //TODO: FIX repeat triggering
  }
  
  //Check for flags
  if (incLitres1)
  {
    litres1++;
    incLitres1 = 0;
  }
  
  if (incLitres2)
  {
    litres2++;
    incLitres2 = 0;
  }
}
//----------------------------------------------------------------------------------------------------------------------

//Control loop that runs at 5 second intervals
//----------------------------------------------------------------------------------------------------------------------
void control()
{ 
    
  Serial.println("DEBUG: RelayState: "+ relayState);    
    
  //Update temperature values    
  temperature1 = analogRead(A0)*330 /4095;
  temperature2 = analogRead(A1)*330 /4095;
  temperature3 = analogRead(A2)*330 /4095;
  temperature4 = analogRead(A3)*330 /4095;
  
  //Read power from the PMIC
  Serial1.write(0x90); 	    //Select page: Page 16
  Serial1.write(0x05);		//Read contents of average power register
  PMIC = (Serial1.read() + (Serial1.read()<<8) + (Serial1.read()<<16))/1242.034;
  
  //Publish data to Spark cloud for debugging and because Wifi is available
  //Spark.publish("data",(String)temperature1+"/"+(String)temperature2+"/"+(String)temperature3+"/"+(String)temperature4+"/"+litres1+"/"+litres2+"/"+PMIC+"/"+drip);
  Spark.publish("data",(String)temperature1+"/"+(String)temperature2+"/"+(String)temperature3+"/"+(String)temperature4+"/"+litres1+"/"+litres2+"/"+PMIC+"/"+drip);
  
  if (modemReset)
  {
      controlMode = 1;
      modemReset = false;
      modemResetCounter++;
      
      if(modemResetCounter > 4)
      {
        //Reset modem
        digitalWrite(A7, LOW);
        delay(1000);
        digitalWrite(A7, HIGH);
        delay(2000);
        
        //Send SMS
        //Serial2.println("AT+CMGS=\"+27715812181\"\rHello. From a friendly modem near you.\x1A");
        
        //Send SMS  
        modemResetCounter = 0;  
      }
  }
  
  //If drip is in any state other than OK (0) force control into do nothing. Last check before entering control loop. Will therefore override any other control signal. 
  if(drip > 0)
  {
    controlMode = 3; 
  }
  
  switch (controlMode)
  {
    case 0:             //Override OFF. Turn relay off
    relaySet (0);
    break;
    
    case 2:             //Override ON. Turn relay ON
    relaySet(1);
    break;
    
    case 1:             //Auto mode
    if (temperature1 > 55)  //Turn element off if temperature greater than 55
    {
         relaySet (0);
    }
    else if (temperature1 < 50) //Turn element on if temperature smaller than 50
    {
        relaySet (1);
    }
    break;
    
    case 3:
    //Do nothing. Valve and relay already set to off in update dripSensor
    break;
    
    default:            //Default to failsafe. Relay off and valve off
    relaySet(0);
    valveSet(0);
    
  }
  
 
  //Increment commsloop counter
  commsLoopCounter++;
  
  //Comms loop
  if (commsLoopCounter > 5)
  {
    //Call comms loop every 25 seconds
    commsLoop();
  }
  
  //Reset elapsed time counter
  elapsedTime  = 0;  
  
  //Reset relay state
  relayState = "OFF";
  relay = 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//Communications loop. Called every minute
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
void commsLoop()
{
  //Declare variables
  byte debugCount = 0;
  String parsedData;

  //Create message buffer 
  String msg;
  msg.reserve(120);
  
  
  //Send message to server
  //--------------------------------------------------------------------------------------------------------------
  //Check whether modem connected to cloud (1) or not (0)
  if(!modemConnected)
  {
    //Send AT message since modem may still be waiting to enter auto UDP mode  
    msg = "AT";
  }
  else
  {
    //Send Status message since it is known that the modem is in UDP mode  
    msg = "{\"e\":\""+relayState+"\",\"t1\":"+(String)temperature1+",\"t2\":"+(String)temperature2+",\"t3\":"+(String)temperature3+",\"t4\":"+(String)temperature4+",\"id\":"+id+",\"valve\":\""+valveState+"\",\"PWR\":"+PMIC+"}";  
  }
      
  //Send message to Debug line and to modem. Message is either "AT" or Status string
  Serial.println("DEBUG: "+ msg);
  Serial2.println(msg);
  
  //Clear contents of msg
  msg = "";
  //-------------------------------------------------------------------------------------------------------------
  
  //Receive response
  //--------------------------------------------------------------------------------------------------------------
  //Delay up to two seconds to allow packet to return
  while(!Serial2.available() && ++debugCount < 20)
  {
    delay (100); //100 Millisecond delay to a maximum of 20 delays = 2 seconds  
  }
  
  //Copy returning string into message
  while(Serial2.available())
  {
    msg += (char)Serial2.read();;
  }
  //---------------------------------------------------------------------------------------------------------------
  
  
  //Decode message loop
  //---------------------------------------------------------------------------------------------------------------
 
  //Message reformatting. Remove quotation marks, new lines, carriage returns, and brackets
  msg.replace("\"","");
  msg.replace("{","");
  msg.replace("}","");
  msg.replace("\r","");
  msg.replace("\n","");
  msg.toUpperCase();
  
  Serial.println("DEBUG: Recieved message: "+ msg);
  
  //Successive checks. Data will fall down the loop if it passes the checks and fall out if it doesn't.      
  
  if(msg.equals("")) //Check that data has been recieved
  {
    //Message timed out. Increment fail counter and exit comms decode loop  
    commsFailCounter++;  
    Serial.println("DEBUG: Message timed out");
  }
  else if(msg.startsWith("OK")) //Check whether the message was a response from the modem to an AT 
  {
    //Modem still in AT mode. Increment fail counter and exit comms decode loop
    commsFailCounter++;
    Serial.println("DEBUG: OK");
  }
  else if(JSONParser("STATUS:",msg,3).equals("CNP")) //Check whether message could be parsed. CNP stands for Could Not Parse
  {
    //Message could not be parsed. Increment fail counter and exit comms decode loop  
    commsFailCounter++; 
    Serial.println("DEBUG: CNP");
  }
  else if (JSONParser("STATUS:",msg,3).equals("ERR")) //Check for error response from server indicating corrupted data
  {
    //Message sent to the server was corrupted. Increment fail counter and exit comms decode loop
    commsFailCounter++;
    Serial.println("DEBUG: ERR");
  }
  else if (JSONParser("STATUS:",msg,3).equals("ACK"))//Additional content to decode
  {
    String temp = JSONParser("E:",msg,3);
    
    if (JSONParser("E:",msg,2).equals("ON"))
    {
        Serial.println("DEBUG: Geyser in ON override");
        //Put unit into AUTO control mode
        controlMode  = 2;
    }
    else if (JSONParser("E:",msg,2).equals("OF"))
    {
        Serial.println("DEBUG: Geyser in OFF override"); 
        //Put unit into AUTO control mode
        controlMode  = 0;
    }
    else if (JSONParser("E:",msg,2).equals("AU"))
    {
        Serial.println("DEBUG: Geyser in AUTO "); 
        //Put unit into AUTO control mode
        controlMode  = 1;
    }
    
    //Reset litre counters - comms was successful so they can be reset
    litres1 = 0;
    litres2 = 0; 
    
    //Reset commsFailCounter. Successful comms achieved
    commsFailCounter = 0;
    modemConnected = true;
  }
  
  // Failed to establish comms five times
  if (commsFailCounter >= 5)
  {
    //Reset modem connected flag
    modemConnected = false;
    //Put unit into AUTO control mode
    controlMode  = 1;
    //Set modem reset flag to 1
    modemReset = true;
    //Reset loopcounter
    commsLoopCounter = 0;
  }

 
}
//------------------------------------------------------------------------------------------------

//Check for burst geyser and shutoff water supply if geyser burst is detected
//------------------------------------------------------------------------------------------------
byte updateDripsensor()
{
  byte val = 2;
  int dripValue = analogRead(A4);
  if (dripValue < 200)
  {
    val = 0;
  }
  else if (dripValue < 3000)
  {
    val = 1;
    //Close supply valve and shutoff power
    valveSet(0);
    relaySet(0);
  }
  else 
  {
    val = 2;
    //Close supply valve and shutoff power
    valveSet(0);
    relaySet(0);
  }
  return val;
}
//------------------------------------------------------------------------------------------------

//Valve open (1) and close (0) method
//------------------------------------------------------------------------------------------------
void valveSet(byte set)
{ 
  Serial.println("DEBUG: Valve: " + (String)set);
  //byte counter = 0;
  
  //Open the valve (1)
  if(set == 1)
  {
    digitalWrite(D4, LOW);
    digitalWrite(D5, HIGH);
    
    /*while(!(digitalRead(D3)) && ++counter < 10)
    {
        delay(10);
    }
    digitalWrite(D5, LOW);
    
    if(counter == 10)
    {
      Serial.println("DEBUG: Valve not set");
      valveState = 0;
    }
    else
    {
      Serial.println("DEBUG: Valve set");    
      valveState = 1;
    }*/
    
    delay(100);
    digitalWrite(D5, LOW);
    valveState = "OPEN";
    
  }
  else //Close the valve (0)
  {
    digitalWrite(D4, HIGH);
    digitalWrite(D5, HIGH);
    
    /*while(!(digitalRead(D3)) && ++counter < 10)
    {
        delay(10);
    }
    digitalWrite(D5, LOW);
    
    if(counter == 10)
    {
      Serial.println("DEBUG: Valve not set"); 
      valveState = 0;
    } 
    else
    {
      Serial.println("DEBUG: Valve set"); 
      valveState = 1;
    }*/
    
    delay(100);
    digitalWrite(D5, LOW);
    valveState = "CLOSED";
    
  }
}
//------------------------------------------------------------------------------------------------

//Count pulses and increment litres when a litre is reached
//------------------------------------------------------------------------------------------------
void incrementCount1()
{
  if(++count2 > 449)
  {
      count2 = 0;
      litres2++;
  }
}
void incrementCount2()
{
  if (++count1 > 449)
  {
      count1 = 0;
      litres1++;
  }       
}
//------------------------------------------------------------------------------------------------

//Set relay state flag to high - gets triggered 50 times a second. Value reset to 0 before 5 second delay begins
//------------------------------------------------------------------------------------------------
void setRelayFlag()
{
  relay = 1;
  relayState = "ON";

}
//------------------------------------------------------------------------------------------------

//Set the relay to either ON (1) or OFF (0)
//------------------------------------------------------------------------------------------------
void relaySet(byte set)
{
      
    
  //if (set != relay)
  //{
    
    Serial.println("DEBUG: Relay: " + (String)set);
    if(set == 1)
    {
      digitalWrite(D7, HIGH);
      delay(100);
      digitalWrite(D7, LOW);
    }
    else
    {
      digitalWrite(D6, HIGH);
      delay(100);
      digitalWrite(D6, LOW);
    }
  //}
}
//------------------------------------------------------------------------------------------------

//Parses JSON by ID and returns the required data
//------------------------------------------------------------------------------------------------
String JSONParser(String ID, String msg, byte valueLength)
{
    String temp = "CNP";
    byte startIndex = msg.indexOf(ID);
    if(startIndex != -1)
    {
        temp = ""+ msg.substring((startIndex + ID.length()),(startIndex + ID.length() + valueLength));
    }
    Serial.println("DEBUG: JSONParser: " + temp);
    return temp;
}
//--------------------------------------------------------------------------------------------------