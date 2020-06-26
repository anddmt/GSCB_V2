// Firmware GSCB V2.0
#include <SoftwareSerial.h>
//#include <stdint.h>
//#include <inttypes.h>
//#include "Arduino.h"

/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        7   // Serial Receive pin
#define SSerialTX        3   // Serial Transmit pin
#define SSerialTxControl 2   // RS485 Direction control
#define Pin13LED         13  // To show communication error or not

/////////////////// SHIFT READ CONFIGURE ////////////////////////

// How many shift register chips are daisy-chained.
#define NUMBER_OF_SHIFT_CHIPS   2
// Width of data (how many ext lines).
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8
// Width of pulse to trigger the shift register to read and latch.
#define PULSE_WIDTH_USEC   10
// Optional delay between shift register reads.
#define POLL_DELAY_MSEC   10000

// Pin to connect with 74HC165
int ploadPin        = 5;  // Connects to Parallel load pin the 165
int dataPin         = 4; // Connects to the Q7 pin the 165
int clockPin        = 6; // Connects to the Clock pin the 165


/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

byte  checkError = 0;

byte SlaveID, SlaveIDold;
byte FunctionSw[] = { 8, 9, 10, 11, 12, 14};
byte inputArr[NUMBER_OF_SHIFT_CHIPS]; // them 2 byte để lưu mã ktra lỗi
byte inputArrOld[NUMBER_OF_SHIFT_CHIPS];
byte SendBuffer[1 + NUMBER_OF_SHIFT_CHIPS +1];  // [slaveID,0xBB + 3byte inputarr]
byte readBuffer[4]; // đọc 3 byte dữ liệu
int   readPoiter=0;

// You will need to change the "int" to "long" If the number of chip higher than 2

// Define variable to save status input
unsigned long  pinValues;      // 4byte
unsigned long  oldPinValues;   // 4byte

//==========================DOC TRANG THAI INPUT ==========================================
void read_input_regs()
{   // Load toan bo input vao thanh ghi dich
    digitalWrite(ploadPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(ploadPin, HIGH);    
    
  for(int k=0; k< NUMBER_OF_SHIFT_CHIPS; k++)
    {
      // Serial.print("Chip 74HC165-:");
       //Serial.print(k+1);
      // Serial.print("\r\n");
     inputArrOld[k] = inputArr[k];
     inputArr[k] = read_one_shift_regs();
    }
  
}  //---------------------------

//Đọc 1 chip ghi dịch và lưu 1 byte
byte read_one_shift_regs()
{
    byte bitVal; // Dung luu tru 01 bit trang thai ghi doc chan ghi dich
    byte byteVal=0;
  
    
   for(int i = 0; i < 8; i++)
    {
       
        
        bitVal = digitalRead(dataPin);

            //Serial.print("/Pin");
            //Serial.print(i+1);
            //Serial.print("=HIGH    ");

        // Set the corresponding bit in bytesVal.
        byteVal |= (bitVal << (7 - i));

        //---Tao 1 xung dich bit-------------
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(clockPin, LOW); 
    }
    //Serial.println("doc 1 byte");
    //Serial.println(byteVal, HEX);
    return(byteVal);
}
//==================
void display_status_pin()
{
  byte bitVal; // Dung luu tru 01 bit trang thai ghi doc chan ghi dich
  
    
  for(int i = 0; i < 2; i++){
      Serial.println(inputArr[i], HEX);
      for (int t = 0; t<8; t++){

       bitVal = inputArr[i]<<t;
        if(bitVal)
        Serial.print("HIGH  ");
        else
          Serial.print("LOW  ");
          
          
      } 
  }   

//byteVal |= (bitVal << (7 - i));
      
}
//====================================================================================
 // Backup PIN status in to other array
 void backup_statusPin()
 {for (int i=0; i < NUMBER_OF_SHIFT_CHIPS; i++)
      inputArrOld[i] = inputArr[i];
 }
 //-------Check pin change ----------------------------------
bool check_pin_change()
{ for (int i=0; i < NUMBER_OF_SHIFT_CHIPS; i++)
  {
    if(inputArrOld[i] != inputArr[i])
     return(true);
  }
  return(false);
}
//-------------------------------------------------------------

//////////////// READ SLAVE ID ON FUNCTION DIPSWITCH ////////////////

byte read_SlaveID()
{
  byte SlaveIDread = 0;
  for (byte j=0; j<6; j++)
  {
    if (digitalRead(FunctionSw[j]) == 0)
    {
      SlaveIDread |= (1 << j);   // Set bit jth on NumSlaveRead
    }
  }
  return (SlaveIDread);
}

//=================================================================================================
//=================================================================================================
////////////////////// SETUP : RUNS ONCE ///////////////////////////

void setup()  
{
  // Start the built-in serial port, probably to Serial Monitor
  Serial.begin(9600);
 
  Serial.print("\r\n");
  
  pinMode(Pin13LED, OUTPUT);   
  pinMode(SSerialTxControl, OUTPUT);  
  digitalWrite(SSerialTxControl, LOW);  // Init Transceiver
  
  // Start the software serial port, to another device
  RS485Serial.begin(4800);   // set the data rate     
  pinMode(ploadPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, INPUT);
   
  digitalWrite(clockPin, LOW);
  digitalWrite(ploadPin, HIGH);
  SendBuffer[1] = 0xBB;  //ma kiem tra loi
      
  SlaveID = read_SlaveID();
  SlaveIDold = SlaveID;
   Serial.println("---- Slave debug tool ----");
  Serial.print("-SlaveID of this Xboard is: ");
  Serial.println(SlaveIDold, HEX);
  Serial.print("\r\n");
  Serial.println("-Trang thai PIN dau vao:");
  backup_statusPin();
  read_input_regs();
  display_status_pin(); 
  
   // Show error status LED   
  digitalWrite(Pin13LED, HIGH);  // LED sang OK, tat NOK   
  
  
   //digitalWrite(Pin13LED, LOW); 
        
}//--(end setup )---

//*****LOOP***********LOOP**************LOOP*************LOOP*********LOOP************LOOP********LOOP******************************************************
////////////////////// LOOP RUN CONTINUE ///////////////////////////

void loop()
{ 
 
  // Read SlaveID set on function dipswitch (3 switch 1,2,3)
 
  SlaveID = read_SlaveID();
  SendBuffer[0]=SlaveID;      // cap nhat dia chi slave bao bo nho phat
  
  while (SlaveID ==0) // chờ set địa chỉ
  {
    Serial.println("Please set SlaveID of this Xboard");
    delay (3000);
    SlaveID = read_SlaveID();
    SendBuffer[0]=SlaveID; 
   }
   
  if (SlaveIDold != SlaveID)
  {
    SlaveIDold = SlaveID;
    Serial.print(">>>>> SlaveID of this Xboard had change to: ");
    Serial.println(SlaveID, HEX);
  }
 //---------------------------------------------------------------------- 
   // delay (1000);
    backup_statusPin();
    read_input_regs(); // Đọc pin đầu vào
    
  // If there was a chage in state, display which ones changed.
  if(check_pin_change())
    {
      Serial.print("*Pin value change detected*\r\n");
      display_status_pin();
      SendBuffer[1] = 0xBB;   
    }
  else  
     SendBuffer[1] = 0xCC;

     if (RS485Serial.available()>0)
     {byte temp=0;
      temp = RS485Serial.read();
      
      Serial.println(temp, HEX);
            if (temp==0xAA){
            Serial.println("Da nhan header mao dau"); 
            readPoiter=0; // reset poiter
            }
         readBuffer[readPoiter] = temp;
         readPoiter++;
         if (readPoiter>=3)
            readPoiter=3;  // gioi han mang
         
     }
   
  RS485Serial.flush() ; //Chờ gửi thông tin qua Serial kết thúc
  // Send data to master
  
  if (readPoiter >=2){
         
      if ((readBuffer[0] == 0xAA)&&(readBuffer[1] == SlaveID)&&(readBuffer[2] == 0x77))
       {  readPoiter=0;
          readBuffer[0]=0;
          readBuffer[1]=0;
          readBuffer[2]=0;
       
        Serial.println("True request from Mboard !!!");
        digitalWrite(SSerialTxControl, HIGH);  // Enable RS485 Transmit 

          SendBuffer[2]=inputArr[0];
          SendBuffer[3]=inputArr[1];
          SendBuffer[4]=inputArr[2];
        // send buffer
        for (int i=0; i< NUMBER_OF_SHIFT_CHIPS +2; i++)
          RS485Serial.write(SendBuffer[i]);
         digitalWrite(SSerialTxControl,LOW);  // Disable RS485 Transmit
        Serial.println("Data had send to Mboard !!!");
       
       }
  }
    
  // Show request on Serial monitor
    
    
  // Check error of comunication
  
  if (checkError >= 10)
   {
    digitalWrite(Pin13LED, LOW);     // Tắt led, Show communication error
    checkError = 0;
   }
   
  if ( checkError == 0)
   {
     digitalWrite(Pin13LED, HIGH);     //Sáng led, Show communication OK
      checkError = 0;
   }

  
}//--(end main loop )---

/*-----( Declare User-written Functions )-----*/
//NONE

//*********( THE END )***********
