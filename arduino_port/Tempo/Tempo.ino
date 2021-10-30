#include "tempo.h"
#include <EEPROM.h>

#define REV_TIME_MIN 10000 // 10ms = 2rev -> 12000RPM
#define REV_TIME_MAX 100000 // 100ms = 2rev -> 1200RPM
#define INJ_TIME_MIN 200 // minima iniezione 200us
#define INJ_TIME_MAX 9000 // massima iniezione 9ms = 9000us
#define IN_INJ_PIN 2 // input pin iniettore da ECU
#define OUT_INJ_PIN 12 // output pin iniettore verso iniettore fisico
#define MAX_PERCENTAGE_INJ 40 // massima percentuale di incremento (per safety)

volatile unsigned int status_inj_in; // valore del pin di iniettore proveniente da centralina (alto o basso)
volatile unsigned long inj_start_prev; // salva il time stamp precedente di iniezione (serve per calcolo RPM)
volatile unsigned long inj_start; // time stamp di inizio iniezione (lo stesso valore di: temp_time) della ECU
volatile unsigned long delta_time; // distanza tra due iniezioni (2rpm)
volatile unsigned long inj_end; // tempo di fine iniezione da ECU
volatile unsigned long delta_inj; // tempo di iniezione input
volatile unsigned long extension_time; // tempo di estensione iniezione, in microsecondi
volatile unsigned int prev_time_val; // validita' (va ad 1 quando ho fatto almeno un ciclo e ho letto il tempo di iniezione, per il calcolo RPM)
volatile unsigned int perc_inc; // percentuale di incremento (letta dalla mappa a seconda di RPM)
volatile unsigned int rpm; // RPM motore calcolate in base alla distanza tra due tempi di iniezione
volatile unsigned int incrementi_rpm_std[]={30,30,32,34,35,35,34,34,32,30,25,25,20,20,20,20}; // standard, riscritte in caso di corruzione EEPROM (write standard values)
volatile unsigned int incrementi_rpm[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // utilizzati dal software
volatile unsigned int throttle; // tensione del throttle pin 0-1023

volatile unsigned long delta_time_buffer; // distanza tra due iniezioni (2rpm) buffer
volatile unsigned long delta_inj_buffer; // tempo di iniezione input buffer
volatile unsigned int throttle_buffer; // tensione del throttle pin 0-1023 buffer
volatile int buffer_busy; // viene attivato quando sta mandando fuori i dati in seriale

char serial_inbyte[]={0,0,0,0,0,0,0,0}; // buffer per dati ricevuti in seriale (8 bytes max)
int serial_byte_cnt; // contatore di numero bytes ricevuti in seriale
char serialdataString[20]; // stringa data fuori in output per il logger
int data_send_req=0; //data sending request coming from logger

void setup() {
  Timer1.initialize();
  pinMode(IN_INJ_PIN, INPUT);
  pinMode(OUT_INJ_PIN, OUTPUT);
  digitalWrite(OUT_INJ_PIN, LOW);
  Serial1.begin(9600);
  while (!Serial1) {
    // wait for serial port to connect. Needed for native USB
  }
  EEPROM_check();
  attachInterrupt(1, signal_change, CHANGE); // interrupt sul pin 2
  status_inj_in=0; // stato ingresso iniettore
  inj_start=0; // tempo inizio iniezione attuale
  inj_start_prev=0; // tempo inizio iniezione precedente
  delta_time=0; // differenza temporale tra due iniezioni (2rpm), attuale meno precedente
  delta_inj=0;
  extension_time=0;
  prev_time_val=0; // validita (non valido perche non ho ancora letto almeno un campione)
  perc_inc=0; // percentuale incremento
  rpm=0; //rpm calcolati motore
  throttle=0; //throttle voltage
  serial_byte_cnt=0; //conto caratteri ricevuti via seriale

  delta_time_buffer=0; // distanza tra due iniezioni (2rpm) buffer
  delta_inj_buffer=0; // tempo di iniezione input buffer
  throttle_buffer=0; // tensione del throttle pin 0-1023 buffer
  buffer_busy=0;
}


void loop() {

  // SERIAL BYTES RECEIVED ARE MANAGED BY THE FOLLOWING LOGIC
  serial_byte_cnt=0;
  while (Serial1.available()) { // carattere ricevuto
    serial_byte_cnt++;
    if (serial_byte_cnt>=9){
      serial_byte_cnt=9; //not valid number (9 bytes is not acceptable)
    }else{ // meno di 9 caratteri, quindi salvo nel buffer
      serial_inbyte[serial_byte_cnt-1]=Serial1.read(); //salvo nel buffer in posizione 0-7 (8 bytes in totale)
    }
    delay(10); // dai il tempo di arrivare al carattere successivo in seriale, qualora ci sia (per evitare comandi troncati a meta' tra un ciclo e successivo)
  }
  if (serial_byte_cnt==8){ // 8 bytes received by the service tool / logger
    if (serial_inbyte[0]=='w' || serial_inbyte[0]=='r' || serial_inbyte[0]=='d'){
      evaluate_parameter_read_writing_request();
    }
  }

  // IN CASE THE SERVICE TOOL / LOGGER REQUESTED A SNAPSHOT, THE ECU REPLIES
  if (data_send_req==1) { // in case the service tool / logger has sent a snapshot request (data "d")
    buffer_busy=1; // buffer busy flag activated, so it cannot be written during this time
    serialdataString[0]='d';                                               // 1 caratteri "d" = data
    serialdataString[1]=14;                                                // 18 = 18 bytes consecutivi
    serialdataString[2]= (byte)(delta_time_buffer & 0xff);
    serialdataString[3]= (byte)((delta_time_buffer >> 8) & 0xff);
    serialdataString[4]= (byte)((delta_time_buffer >> 16) & 0xff);
    serialdataString[5]= (byte)((delta_time_buffer >> 24) & 0xff);          // 4 caratteri
    serialdataString[6]= (byte)(delta_inj_buffer & 0xff);
    serialdataString[7]= (byte)((delta_inj_buffer >> 8) & 0xff);
    serialdataString[8]= (byte)((delta_inj_buffer >> 16) & 0xff);
    serialdataString[9]= (byte)((delta_inj_buffer >> 24) & 0xff);           // 4 caratteri
    serialdataString[10]= (byte)(throttle_buffer & 0xff);
    serialdataString[11]= (byte)((throttle_buffer >> 8) & 0xff);             // 2 caratteri
    unsigned long crc_somma;
    crc_somma=delta_time_buffer+delta_inj_buffer+throttle_buffer;
    serialdataString[12]= (byte )(crc_somma & 0xff);
    serialdataString[13]= (byte)((crc_somma >> 8) & 0xff);
    serialdataString[14]= (byte)((crc_somma >> 16) & 0xff);
    serialdataString[15]= (byte)((crc_somma >> 24) & 0xff);          // 4 caratteri
    Serial1.write(serialdataString, 16);
    data_send_req=0; // resets the sending request
    buffer_busy=0; // reset the buffer busy flag, so that the it can be updated
  }

  delay(10); // delay ms
}

unsigned int evaluate_perc_inc_map(){
  unsigned int index = rpm/1000;
  unsigned int rimanente = rpm-1000*index;
  if (index>=0 && index<15){
    unsigned int percentuale=incrementi_rpm[index]*(1000-rimanente)/1000+incrementi_rpm[index+1]*rimanente/1000;
    return percentuale;
  }
  if (index>=15){ // saturato al massimo
    return incrementi_rpm[15];
  }
  return 0;
}

void update_info_for_logger(){
  if (!buffer_busy){
    delta_time_buffer=delta_time; // distanza tra due iniezioni (2rpm) buffer
    delta_inj_buffer=delta_inj; // tempo di iniezione input buffer
    throttle_buffer=throttle; // tensione del throttle pin 0-1023 buffer
  }
}

void deactivate_inj() // disattiva l'iniettore quando il timer scatta
{
  Timer1.stop(); // stoppa il timer
  Timer1.detachInterrupt(); // stacca l'interrupt
  digitalWrite(OUT_INJ_PIN, LOW); // disattiva l'output
  update_info_for_logger();
}


void signal_change() // cambia il segnale di iniettore in ingresso
{
  status_inj_in=digitalRead(IN_INJ_PIN);
  if(status_inj_in){ //iniettore viene attivato dalla ECU
    inj_start=micros();
    digitalWrite(OUT_INJ_PIN, HIGH);
    throttle = analogRead(A0); // legge tensione del pin throttle (analog input A0)
    if(prev_time_val==1){ // se c'e' stato almeno un ciclo precedente
      delta_time=inj_start-inj_start_prev; // calcolo tempo tra due iniezioni consecutive (2 rpm)
      if (delta_time>REV_TIME_MIN && delta_time<REV_TIME_MAX){
        rpm=120*1000000/delta_time;
      }else{
        rpm=0; // il tempo tra due iniezioni non e' nel range accettabile
      }
    }else{
      rpm=0; // non e' possibile calcolare rpm perche' non ho fatto almeno un ciclo
    }
    perc_inc=incrementi_rpm[rpm/1000]; // legge il tempo di incremento da mappa
    //perc_inc=evaluate_perc_inc_map();
    if (perc_inc>MAX_PERCENTAGE_INJ) perc_inc=MAX_PERCENTAGE_INJ; //limita il tempo di iniezione verso l'alto, per safety
    inj_start_prev=inj_start; // salva il valore di time stamp di iniezione come valore old
    prev_time_val=1; // validita positiva perche ho letto un campione e l'ho salvato in old
  }else{ //iniettore viene disattivato dalla ECU
    inj_end=micros();
    delta_inj=inj_end-inj_start;
    if (delta_inj>INJ_TIME_MIN && delta_inj<INJ_TIME_MAX){ // controlla che il tempo di iniezione misurato sia nel range accettabile
      extension_time=delta_inj*perc_inc/100; // calcola il tempo di iniezione in microsecondi
      //Timer1.setPeriod(extension_time-INJ_OFFSET);
      Timer1.setPeriod(extension_time); // setta il timer
      Timer1.attachInterrupt(deactivate_inj); // attacca l'interrupt
      Timer1.start(); // fa partire il timer
    }else{
      digitalWrite(OUT_INJ_PIN, LOW); // stacca subito in caso di tempo di iniezione misurato strano
      extension_time=0;
      update_info_for_logger();
    }
  }
}


int evaluate_parameter_read_writing_request()
{
  int temp_num=0;
  int sum_rpm=0;
  int sum_throttle=0;
  int sum_value=0;
  int i=0;
  
  temp_num=serial_inbyte[1]-'0';
  sum_rpm+=10*temp_num;
  temp_num=serial_inbyte[2]-'0';
  sum_rpm+=temp_num;
  
  temp_num=serial_inbyte[3]-'0';
  sum_throttle+=10*temp_num;
  temp_num=serial_inbyte[4]-'0';
  sum_throttle+=temp_num;
  
  temp_num=serial_inbyte[5]-'0';
  sum_value+=100*temp_num;
  temp_num=serial_inbyte[6]-'0';
  sum_value+=10*temp_num;
  temp_num=serial_inbyte[7]-'0';
  sum_value+=temp_num;
  
  if(sum_rpm>=0 && sum_rpm<16){
    if(sum_throttle>=0 && sum_throttle<16){
      String singleMessageString = "";
      if(sum_value>=0 && sum_value<MAX_PERCENTAGE_INJ){
        if (serial_inbyte[0]=='w'){
          incrementi_rpm[sum_rpm]=sum_value;
          EEPROM.write(sum_rpm, sum_value); //for redundancy
          EEPROM.write(sum_rpm+16, sum_value+16); //for redundancy
          sum_value=EEPROM.read(sum_rpm);
          singleMessageString+="w";
          if (sum_rpm<10) singleMessageString+="0";
          singleMessageString+=sum_rpm;
          singleMessageString+="00";
          if (sum_value<10) singleMessageString+="0";
          if (sum_value<100) singleMessageString+="0";
          singleMessageString+=sum_value;
          Serial1.print(singleMessageString); // manda fuori il valore appena scritto in EEPROM
          return 0;
        }
        if (serial_inbyte[0]=='r' && sum_value==0){
          singleMessageString+="r";
          if (sum_rpm<10) singleMessageString+="0";
          singleMessageString+=sum_rpm;
          singleMessageString+="00";
          sum_value=incrementi_rpm[sum_rpm];
          if (sum_value<10) singleMessageString+="0";
          if (sum_value<100) singleMessageString+="0";
          singleMessageString+=sum_value;
          Serial1.print(singleMessageString); // manda fuori il valore appena letto da EEPROM
          return 0;
        }
      }
    }
  }
  if(sum_rpm==16 && sum_throttle==16 && sum_value==0 && serial_inbyte[0]=='r'){ //r1616000 = read all values
    EEPROM_read();
    return 0;
  }
  if(sum_rpm==16 && sum_throttle==16 && sum_value==1 && serial_inbyte[0]=='r'){ //r1616001 = EEPROM check
    if (EEPROM_check()){ // =1 means OK
      Serial1.print("r1616000"); //EEPROM OK
    }else{ // =0 means NG (EEPROM has been rewritten to standard values)
      Serial1.print("w1616000"); //EEPROM ERROR, write was needed and performed
    }
    return 0;
  }
  if(sum_rpm==16 && sum_throttle==16 && sum_value==0 && serial_inbyte[0]=='w'){ //w1616000 = write standard values
    EEPROM_write_standard_values();
    Serial1.print("w1616000"); //write was performed
    return 0;
  }
  if(sum_rpm==16 && sum_throttle==16 && sum_value==0 && serial_inbyte[0]=='d'){ //d1616000 = read current data shapshot
    data_send_req=1; // requires the loop to send the data
    return 0;
  }
  return 1;
}


int EEPROM_check()
{
  int i=0;
  int error_flag=0;
  for (i=0; i<16; i++){
    if (EEPROM.read(i) != (EEPROM.read(i+16)-16)){
      error_flag=1;
    }else{
      incrementi_rpm[i]=EEPROM.read(i);
    }
  }
  if (error_flag==1){
    EEPROM_write_standard_values();
    return 0; // check NG
  }else{
    return 1; // check OK  
  }
  return 0;
}


int EEPROM_read(){ // questa funzione ha 100ms * 16 = 1600ms di delay totale, quindi richiede circa 1s, considerando anche il tempo di lettura da EEPROM
  int i=0;
  int single_read=0;
  String singleReadString = "";
  for (i=0; i<16; i++){
    singleReadString.remove(0);
    single_read=EEPROM.read(i);
    singleReadString+="r";
    if (i<10) singleReadString+="0";
    singleReadString+=i;
    singleReadString+="00";
    if (single_read<10) singleReadString+="0";
    if (single_read<100) singleReadString+="0";
    singleReadString+=single_read;
    Serial1.print(singleReadString);
    delay(100); // per evitare di mandare in overload il logger, che riceve i dati via seriale
  }
  return 0;
}


int EEPROM_write_standard_values(){ // scrive valori standard in EEPROM (in caso di errore di lettura, o in caso di richiesta via seriale "w1616000")
  int i=0;
  for (i=0; i<16; i++){
      EEPROM.write(i, incrementi_rpm_std[i]); //main data
      EEPROM.write(i+16, incrementi_rpm_std[i]+16); //for redundancy, this is a copy of the main data
      incrementi_rpm[i]=incrementi_rpm_std[i]; // prende il valore standard
  }
  return 0;
}
