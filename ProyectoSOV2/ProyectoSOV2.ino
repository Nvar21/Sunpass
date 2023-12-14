#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <Stepper.h>
#include <ArduinoJson.h>

/*---------------------------------------------------------------------*/
/*-----------------------------Constantes------------------------------*/
const int stepsPerRevolution = 2048;  //cantidad de steps por revolución
const int sensorPin = 34;  // Número de pin analógico en ESP32

// ULN2003 Motor Driver Pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17

int ubicacionID = 18;
//mis credenciales de red
const int botonPin = 2; // Cambia el número del pin según tu conexión

const char* ssid = ".TigoWiFi-684727/0";
const char* password = "WiFi-80623736";
const String api_key = "ab7d6d4c52224d0d95bc4f1830eb8e49"; // Reemplaza con tu clave de API de ipgeolocation.io
//api key por si aca me como la otra>>>>>> 5de00fb4ef114cb9a326d10faec73e57
//para ldr
const int ldrPin = 35;
// Constantes de la ubicación
float latitud = 37.77490000; 
float longitud = -122.41940000;

  // Extraer la información de la respuesta JSON
  String dia;
  String amanecer; // Amanecer
  String atardecer; // Atardecer
  String elevacionSolar = "0.00";
  String azimutSolar = "0.00";
  String horaActual; // hora actual
  String hora; // extrae el valor de la hora de horaActual (00:00:00.000)>>>(00:00)
  String duracionDia; // horas totales de dia
  String horasDia;
  String minutosDia;
  int minutosDiaTotal;

/*---------------------------------------------------------------------*/
/*-------------------------Inicializaciones----------------------------*/
// libreria para el stepper motor 
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);
// para preferencias (memoria no volatil)
Preferences preferences;
// para usar el lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);

/*---------------------------------------------------------------------*/
/*----------------------------Variables--------------------------------*/
int anguloFin;
// para el sensor de voltaje
int sensorValue = 0;
int anguloActual = preferences.getInt("anguloActual", 90);
int valorLDR;
int usuario_id = 18;
// Para la conexion

// setup se ejecuta una sola vez
void setup() {
  // velocidad del motor a 5rpm
  myStepper.setSpeed(5);
  // inicializa puerto serial para comunicación serial con la pc
  Serial.begin(115200);

  // Abre la partición de preferencias con un nombre específico
  preferences.begin("SunPass", false);  // El segundo parámetro indica si se debe borrar la partición (true) o no (false)

  // Lee una variable de la memoria no volátil, obtiene el ultimo angulo del motor
  int angulo = preferences.getInt("anguloActual", 90);

  lcd.begin();  // Inicializa el LCD con 20 columnas y 4 filas
  //lcd.backlight();   // Enciende la retroiluminación
  lcd.setCursor(0, 0);
  lcd.print("Iniciando SunPass!");
  lcd.setCursor(0, 1);
  lcd.print("Conectando...");


  // Conectar a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");
}

void loop() {
  Serial.println("-----------------------------------");
  ObtenerCoordenadas();
  Serial.println("-----------------------------------");
  obtenerHoraDesdeAPI();
  delay(1000);
  buscarAngulo(elevacionSolar.toFloat());
  delay(1000);
  CargarMediciones();
  delay(60000); //60 segundos de espera entre cada loop
  Serial.println("-----------------------------------");
  Serial.println("-----------------------------------");

    // Verificar el estado del botón
  int estadoBoton = digitalRead(botonPin);

  // Si el botón está presionado, ejecutar la función configurarPosicion
  if (estadoBoton == HIGH) {
    configurarPosicion();
    delay(1000); // Añadir un pequeño retardo para evitar múltiples ejecuciones por una sola pulsación
  }
}

void obtenerHoraDesdeAPI() {
    lcd.clear();
  // Realizar la solicitud HTTP a ipgeolocation
  String url = "https://api.ipgeolocation.io/astronomy?apiKey="+api_key+"&lat="+latitud+"&long="+longitud;
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.println("Obteniendo hora desde API...");
  Serial.print("URL Api ipgeolocation:");
  Serial.println(url);

  // Verificar si la solicitud fue exitosa
  if (httpCode > 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());

    // datos de la respuesta JSON
    dia = doc["date"].as<String>();
    amanecer = doc["sunrise"].as<String>();
    atardecer = doc["sunset"].as<String>();
    elevacionSolar = doc["sun_altitude"].as<String>();
    azimutSolar = doc["sun_azimuth"].as<String>();
    horaActual = doc["current_time"].as<String>();
    duracionDia = doc["day_length"].as<String>();

    anguloFin = elevacionSolar.toFloat();

    horasDia = duracionDia.substring(0,2);
    minutosDia = duracionDia.substring(3);
    minutosDiaTotal = horasDia.toInt() * 60 + minutosDia.toInt();
    Serial.print("-- Horas de Día de la ubicación: ");
    Serial.println(duracionDia);
    Serial.println("-- Fecha: "+ dia);
    Serial.println("-- hora actual: "+ horaActual);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Hora: "+horaActual.substring(0));
    lcd.setCursor(0,1);
    lcd.print("Fecha: "+dia.substring(0));
    lcd.setCursor(0,2);
    lcd.print("Elev. Sol: " + elevacionSolar.substring(0, 6));
      lcd.setCursor(0,3);
    lcd.print("Dur. Dia:" + duracionDia.substring(0)+"horas");
  } else {
    lcd.clear();
    lcd.setCursor(0,2);
    lcd.print("Error en la solicitud HTTP");
    lcd.setCursor(2,3);
  }

  http.end(); // Liberar recursos
}

float medirTension(){
  int sensorValue = analogRead(sensorPin);
  float voltaje = (sensorValue / 987.8) * 5;//calibración para convertir valor obtenido del sensor a >voltaje< del panel
    Serial.println("");
  Serial.println("--------- Voltaje Medido del panel ---------");
  Serial.print("Voltaje del panel solar: ");
  Serial.println(voltaje);
  return voltaje;
}

String medirLDR(){
 valorLDR = analogRead(ldrPin);
 String nivelLuz ="Bajo";
   Serial.println("------- Nivel de luz -------");
  Serial.println("Lectura del LDR: " + String(valorLDR));
  if (valorLDR > 4000) {
    Serial.println("Lectura del LDR es mayor de 4000");
    nivelLuz = "Excelente";
  } else if (valorLDR > 3000) {
    Serial.println("Lectura del LDR es mayor de 3000");
    nivelLuz = "Bueno";
  } else if (valorLDR > 2000) {
    Serial.println("Lectura del LDR es mayor de 2000");
    nivelLuz = "Regular";
  } else if (valorLDR <= 1000) {
    Serial.println("Lectura del LDR es menor o igual a 1000");
    nivelLuz = "Bajo";
  }
  return nivelLuz;
  Serial.println("Nivel de luz: " + nivelLuz);
}


void CargarMediciones() {
  String URL = "http://192.168.0.11/SUNPASS_SO/subirMediciones.php";
  float voltajePanel = medirTension();
  String nivelLuz = medirLDR();
  String cargaMediciones = "voltaje=" + String(voltajePanel) + "&ldr='" + nivelLuz + "'&ubicacion_id=" + String(ubicacionID) + "&hora=" + horaActual + "&dia=" + dia;
  Serial.println(cargaMediciones);

  HTTPClient http;
  String requestURL = URL + "?" + cargaMediciones;  // Agregar los datos a la URL
  http.begin(requestURL);  // Configurar la solicitud

  int httpCode = http.GET();  // Realizar solicitud GET

  Serial.print("HTTP Code cargar BD: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println("Respuesta: " + http.getString());
  } else {
    Serial.println("Error en la solicitud HTTP");
  }

  http.end();  // Liberar recursos
}

void buscarAngulo(float angulo_fin) {
  
  float stepsXangulo = 5.68;
  anguloActual = preferences.getInt("anguloActual", 90);
  Serial.println("Angulo de Actual:" +anguloActual);
  if (angulo_fin < 0 ){
    angulo_fin = 0;
  } else if (angulo_fin > 180){
    angulo_fin = 180;
  }

  if (angulo_fin > anguloActual) {
    int forwSteps = angulo_fin - anguloActual;
    for (int i = 0; i < forwSteps; i++) {
      myStepper.step(+stepsXangulo);
      anguloActual++;
      preferences.putInt("anguloActual", anguloActual);
      delay(5);
    }
  } else if (angulo_fin < anguloActual) {
    int returnSteps = anguloActual - angulo_fin;
    for (int i = 0; i < returnSteps; i++) {
      myStepper.step(-stepsXangulo);
      anguloActual--;
      preferences.putInt("anguloActual", anguloActual);
      delay(5);
    }
  } else {
    Serial.println("mismo ángulo");
  }
}

void ObtenerCoordenadas() {
  String URL = "http://192.168.0.11/SUNPASS_SO/obtenerCoordenadas.php";
  HTTPClient http;
  String requestURL = URL + "?ubicacion_id=" + ubicacionID;
  Serial.println("--------id de ubicacion:" + ubicacionID);
  //Serial.println("------------URL:" + requestURL);

  http.begin(requestURL);

  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("Solicitud HTTP exitosa");

    // Obtén la respuesta JSON
    String jsonString = http.getString();
    //Serial.println("JSON recibido: " + jsonString);

    // Crea un objeto JSON
    DynamicJsonDocument jsonDocument(1024);
    DeserializationError error = deserializeJson(jsonDocument, jsonString);

    // Verifica errores de deserialización
    if (error) {
      Serial.println("Error al deserializar JSON");
      Serial.println(error.c_str());
    } else {
      // Extrae los valores y asígnales a las variables globales
      const char* latitudStr = jsonDocument[0]["latitud"];
      const char* longitudStr = jsonDocument[0]["longitud"];

      // Convierte las cadenas a flotantes
      latitud = atof(latitudStr);
      longitud = atof(longitudStr);
    }
  } else {
    Serial.println("Error en la solicitud HTTP");
    Serial.println(httpCode);
  }

  http.end();
}

void configurarPosicion() {
  Serial.println("Botón presionado - Configurando posición");

  // Solicitar al usuario que ingrese un grado
  Serial.print("Ingrese el grado de posición deseado: ");
  
  while (!Serial.available()) {
    // Esperar a que el usuario ingrese el grado
  }

  // Leer el grado ingresado por el usuario
  int gradoIngresado = Serial.parseInt();

  // Verificar si se ingresó un valor válido
  if (Serial.read() == '\n') {
    // Realizar la lógica específica con el grado ingresado
    Serial.print("Configurando posición a ");
    Serial.print(gradoIngresado);
    Serial.println(" grados");

    // ... (agrega aquí tu lógica específica con el grado)
  } else {
    Serial.println("Error al ingresar el grado. Asegúrese de ingresar un valor numérico.");
  }
}




