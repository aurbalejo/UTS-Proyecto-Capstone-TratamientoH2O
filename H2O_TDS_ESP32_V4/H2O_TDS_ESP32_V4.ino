/*
 * H2O Sistema de Monitoreo de Dureza de Agua
 * Por: ARTURO URBALEJO, TADEO PORTELA
 * Fecha: 20 de JULIO de 2022
 * 
 * Características
 * Automatizar el proceso de ósmosis inversa del sistema de
 * purificación de agua de la UTS

  *Sensor de temperatura  ............(GPIO13)
  *Sendor de Dureza de H2O TDS .......(GPIO36)
  *Válvula Agua con Solidos  .........(GPIO23)
  *Válvula Agua DUREZA menor a 40 ppm (GPIO18)
  *Bomba de agua  ....................(GPIO21)
 */

// Bibliotecas
#include <WiFi.h>  // Biblioteca para el control de WiFi
#include <PubSubClient.h> //Biblioteca para conexion MQTT
#include <OneWire.h> // Para el manejo del sensor DS18b20
#include <DallasTemperature.h> // Para el manejo del sensor DS18b20

//Datos de WiFi
const char* ssid = "IoT";  // Aquí debes poner el nombre de tu red
const char* password = "cursoiot";  // Aquí debes poner la contraseña de tu red

//Datos del broker MQTT
const char* mqtt_server = "192.168.2.6"; // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica
IPAddress server(192,168,2,6);

// Objetos
WiFiClient espClient; // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient); // Este objeto maneja los datos de conexion al broker
#define ONE_WIRE_BUS 13    //Para sensor de temperatura
#define TdsSensorPin 36    //Para sensor de Dureza de agua
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Constantes
const int pinValLimpia = 18; // Pin que activa electroválvula con dureza menor a 40 ppm
const int pinValSolido = 23;  // Pin que activa electroválvula de agua con dureza alta
const int pinBomba = 21;  // Pin que activa bomba de agua
const int statusLedPin = 2;  // Estado de conexion con la Red Wifi

// Variables
float temperature = 25,tdsValue = 0, tempReal=18; //Variables para el sensor
float voltajePromedio=0, CompCoef, compVoltaje; // Variables para caracterizar sensor
long timeObjetivo, timeActual, timeObjetivo2; // Variables de control de tiempo no bloqueante
long timeObjSim1, timeObjSim2;  // tiempo de simulación de ajuste
float tdsS=0, tdsC=0, tds=0;
int aguaSolida, aguaLimpia, varManuAuto, varBomba; // Para tomar deciciones que llegan de Node-Red 
int simPaso1, simPaso2, simPaso3; // Para simular proceso
int espera = 8000;  // tiempo para envío de mensajes MQTT

//  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Condiciones iniciales - Se ejecuta sólo una vez al energizar
void setup() {// Inicio de void setup ()
  Serial.begin (115200);  // Se inicia comunicación serial a 115200 baudios
  Serial.println("Conectado... ");
  pinMode(TdsSensorPin,INPUT);
  adcAttachPin(TdsSensorPin);
  sensors.begin();
  pinMode (statusLedPin, OUTPUT);  // Configurar pin como salida
  pinMode (pinValLimpia, OUTPUT);  // Configurar pin como salida
  pinMode (pinValSolido, OUTPUT);  // Configurar pin como salida
  pinMode (pinBomba, OUTPUT);      // Configurar pin como salida
  

 //Inicia conexion a WiFi
  Serial.println();
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);
 delay(1000);
  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi
 
  while (WiFi.status() != WL_CONNECTED) { // Este bucle espera a que se realice la conexión
    digitalWrite (statusLedPin, HIGH);
    delay(500); //dado que es de suma importancia esperar a la conexión, debe usarse espera bloqueante
    digitalWrite (statusLedPin, LOW);
    Serial.print(".");  // Indicador de progreso
    delay (5);
  }
  
  // Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());   //Muestra la dirección IP del la ESP32

// Si se logro la conexión, encender led
  if (WiFi.status () > 0){
  digitalWrite (statusLedPin, LOW);
  }

  delay (1000); // Esta espera es solo una formalidad antes de iniciar la comunicación con el broker

  // Conexión con el broker MQTT
  client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
  client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos
 
  delay(1500);  // Esta espera es preventiva, espera a la conexión para no perder información

  timeActual = millis (); // Variable que contiene el tiempo en milisegundos de cada ciclo
  timeObjetivo = timeActual + 4000; // tiempo objetivo de 4 seg
  timeObjetivo2 = timeActual + 8000;// tiempo objetivo de 8 seg
  aguaSolida =1;
  aguaLimpia =1;
  varManuAuto=0;
  simPaso1=0;
  simPaso2=0; 
  simPaso3=0;
  // Debes subcribirte a los topic que envía NODE-RED
  client.subscribe("CodigoIoT/SIC/G5/H2O/valSolidos"); // Esta función realiza la suscripción al tema
  client.subscribe("CodigoIoT/SIC/G5/H2O/valLimpia"); // Esta función realiza la suscripción al tema
  client.subscribe("CodigoIoT/SIC/G5/H2O/manuAuto");
  client.subscribe("CodigoIoT/SIC/G5/H2O/bomba"); 
}// Fin de void setup


//  ***********************************************************

// Cuerpo del programa - Se ejecuta constamente
void loop() {// Inicio de void loop

//Verificar siempre que haya conexión al broker
  if (!client.connected()) {
    reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
  }// fin del if (!client.connected())
  client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker
  
   timeActual = millis(); // Variable que contiene el tiempo en milisegundos de cada ciclo
   lecturaSensores();  // Función de lectura de sensores
   lecturaSensorTDS(); // Función que envía al Broker la dureza del agua
   lecturaSensorTemperatura(); // Función que envía al broker la Temperatura

// Condiciones para operación Manual
   if (varManuAuto == 0){  
         simPaso1=0, simPaso2=0,simPaso3=0;
         if ( aguaSolida == 0) {
             digitalWrite(pinValLimpia, LOW);
         }
         else {
             digitalWrite(pinValLimpia, HIGH);
         }
      
         if (aguaLimpia == 0) {
             digitalWrite(pinValSolido, LOW);
         }
         else {
             digitalWrite(pinValSolido, HIGH);
         }
      }  // Fin del modo Manual

// Simulacion del proceso de ósmosis inversa

    if (varManuAuto == 1) {

      if (simPaso1==0){
        timeObjSim1=timeActual + 45000;
        digitalWrite(pinVallimpia, HIGH);
        digitalWrite(pinValSolido, LOW);
        delay(100);
        simPaso1=1;
      }

      if (simPaso1 == 1 && simPaso2 == 0 && timeActual >= timeObjSim1) {
        digitalWrite(pinValSolido, LOW);
        digitalWrite(pinValLimpia, LOW);
        delay(100);
        timeObjSim2=timeActual + 40000;
        simPaso2 = 1;
      }

      if (simPaso3==0 && simPaso2==1 && timeActual >= timeObjSim2){
        timeObjSim1=timeActual + 5000;
        digitalWrite(pinValSolido, HIGH);
        digitalWrite(pinValLimpia, LOW);
        delay(100);
        simPaso3=1;
      }
     if (simPaso3==1 && timeActual >= timeObjSim1){
        digitalWrite(pinValLimpia, HIGH);
        digitalWrite(pinValSolido, HIGH);
     }
   }  // Fin del modo automático
   
   //Encendido y apagado de la Bomba
   if (varBomba == 1){
      digitalWrite(pinBomba, LOW);
   }
   else{
      digitalWrite(pinBomba, HIGH);
   }
}                     // Fin de void loop
//  ****************************************************************

//++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* message, unsigned int length) {

  // Concatenar los mensajes recibidos para conformarlos como una varialbe String
  String messageTemp; // Se declara la variable en la cual se generará el mensaje completo  
  for (int i = 0; i < length; i++) {  // Se imprime y concatena el mensaje
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  // Ejemplo, en caso de recibir el mensaje true - false, se cambiará el estado del led soldado en la placa.
  // El ESP32 está suscrito al tema
  if (String(topic) == "CodigoIoT/SIC/G5/H2O/valSolidos") {  // En caso de recibirse mensaje en el tema
    if(messageTemp == "true"){
      Serial.println("Activar valvula de solidos");
      aguaSolida = 0;
    }// fin del if (String(topic) == "")
    else if(messageTemp == "false"){
      Serial.println("Desactivar valvula de solidos");
      aguaSolida = 1;
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "")


  if (String(topic) == "CodigoIoT/SIC/G5/H2O/valLimpia") {  // En caso de recibirse mensaje en el tema
    if(messageTemp == "true"){
      Serial.println("Activar valvula de Limpia");
      aguaLimpia = 0;
    }// fin del if (String(topic) == "")
    else if(messageTemp == "false"){
      Serial.println("Desactivar valvula de Limpia");
      aguaLimpia = 1;
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "")

  if (String(topic) == "CodigoIoT/SIC/G5/H2O/manuAuto") {  // En caso de recibirse mensaje en el tema
    if(messageTemp == "true"){
      Serial.println("Activar modo Automático");
      varManuAuto = 1;
    }
    else if(messageTemp == "false"){
      Serial.println("Desactivar modo Automático");
      varManuAuto = 0;
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == )

  if (String(topic) == "CodigoIoT/SIC/G5/H2O/bomba") {  // En caso de recibirse mensaje en el tema
    if(messageTemp == "true"){
      Serial.println("Activar BOMBA");
      varBomba = 1;
    }
    else if(messageTemp == "false"){
      Serial.println("Desactivar BOMBA");
      varBomba = 0;
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == )
    
  delay(100);
}// fin del void callback

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Funcioes del usuario

//LECTURA DE SENSORES
void lecturaSensores(){
    sensors.requestTemperatures();
    tempReal = sensors.getTempCByIndex(0);
    
    tds = analogRead(TdsSensorPin);
    tdsS = tds*(0.244200244200244);  // 1000/4095
    if (tdsS<20){
      tdsValue=20;
    }
    else if (tdsS < 100) {
      tdsValue=tdsS;
    }
    else {
     voltajePromedio=tds*5.0/4095;
     CompCoef=1.0 + 0.02*(tempReal-25.0);
     compVoltaje=voltajePromedio/CompCoef;
     tdsValue=(133.42*compVoltaje*compVoltaje*compVoltaje-255.86*compVoltaje*compVoltaje+857.39*compVoltaje)*0.5;
    }
}

//LECTURA SENSOR  T D S
void lecturaSensorTDS(){

  if (timeActual >= timeObjetivo ) { // Manda un mensaje por MQTT cada cinco segundos
    timeObjetivo = timeActual + espera; // Actualización de seguimiento de tiempo
    
    Serial.print("Sensor =");
    Serial.print(tds);
    Serial.print(" ==> ");    
     Serial.print(tdsValue);
     Serial.print(" ppm");
     
     char dataString[8]; // Define una arreglo de caracteres para enviarlos por MQTT, especifica la longitud del mensaje en 8 caracteres    
     dtostrf(tdsValue, 4, 0, dataString);  // Esta es una función nativa de leguaje AVR que convierte un arreglo de caracteres en una variable String
     client.publish("CodigoIoT/SIC/G5/H2O/ppm", dataString);
    
  }
 
}

// LECTURA SENSOR DE TEMPERATURA
void lecturaSensorTemperatura(){

  if (timeActual >= timeObjetivo2 ) { // Manda un mensaje por MQTT cada cinco segundos
    timeObjetivo2 = timeActual + espera; // Actualización de seguimiento de tiempo

     Serial.print("  Temperatura: ");
     Serial.println(tempReal);
    
     char dataString[8]; // Define una arreglo de caracteres para enviarlos por MQTT, especifica la longitud del mensaje en 8 caracteres    
     dtostrf(tempReal, 2, 2, dataString);  // Esta es una función nativa de leguaje AVR que convierte un arreglo de caracteres en una variable String
     client.publish("CodigoIoT/SIC/G5/H2O/temp", dataString);     
     
  }  // Fin del  if (timeActual - timeObjetivo2 )

}


// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      // Esta función realiza la suscripción al tema
      client.subscribe("CodigoIoT/SIC/G5/H2O/valSolidos"); 
      client.subscribe("CodigoIoT/SIC/G5/H2O/valLimpia");
      client.subscribe("CodigoIoT/SIC/G5/H2O/manuAuto");
      client.subscribe("CodigoIoT/SIC/G5/H2O/bomba");
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      delay(5000);  // Espera de 5 segundos bloqueante
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect
