#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <Stepper.h>
#include <ArduinoJson.h>

/*---------------------------------------------------------------------*/
/*-----------------------------Constantes------------------------------*/
//Para el stepper
const int stepsPerRevolution = 2048;  //cantidad de steps por revolución
const int sensorPin = 34;  // Número de pin analógico en ESP32
// ULN2003 Motor Driver Pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17

//para ldr
const int ldrPin = 35;

//mis credenciales de red
const char* ssid = ".TigoWiFi-684727/0";
const char* password = "WiFi-80623736";

// Constantes de la ubicación
String latitud = "9.01267"; 
String longitud = "-79.51428";
const String api_key = "7e5b55322ea74fe39d72eb3da222d222"; // Reemplaza con tu clave de API de ipgeolocation.io

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
int ubicacionID = preferences.getInt("ubicacionActual", 1);
// para el sensor de voltaje
int sensorValue = 0;
int anguloActual = preferences.getInt("anguloActual", 90);
int valorLDR;
int usuario_id = 1;
int ubicacion_id;
// Para la conexion
String estado_conexion="Desconectado";
// Para angulos de motores


// Extraer la información de la respuesta JSON
String dia ;
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
    estado_conexion ="Desconectado";
  }
  Serial.println("Conectado a WiFi");
  estado_conexion ="Conectado";
}
  

void loop() {
  //obtiene la ubicacion/amanecer/atardecer/elevacionSolar
  //
  CambiarUbicacion();
  //CambiarUbicacion();
  delay(10000);
  // lcd.clear();
  // obtenerHoraDesdeAPI();
  // medirTension();
  // for (int i = 0;1<100;i++){
  //   medirLDR();
  // }
  // Tu código del bucle aquí
}


void obtenerHoraDesdeAPI() {
    lcd.clear();
  // Realizar la solicitud HTTP a ipgeolocation
  String url = "https://api.ipgeolocation.io/astronomy?apiKey="+api_key+"&lat="+latitud+"&long="+longitud;
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.println("Obteniendo hora desde API...");
  Serial.print("La solicitud es: ");
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

    horasDia = duracionDia.substring(1,2);
    minutosDia = duracionDia.substring(4,5);
    minutosDiaTotal = horasDia.toInt() * 60 + minutosDia.toInt();
    Serial.println("minutos totales de día: "+ minutosDiaTotal);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Hora: "+hora.substring(0));
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
    lcd.print("Conexion: "+estado_conexion);
  }

  http.end(); // Liberar recursos
  delay(100);
}

void buscarAngulo(int angulo_fin) {
  float stepsXangulo = 5.68;
  anguloActual = preferences.getInt("anguloActual", 90);
  
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
    Serial.println("Es el mismo ángulo en el que ya estoy");
  }
}


float medirTension(){
  int sensorValue = analogRead(sensorPin);
  float voltaje = (sensorValue / 987.8) * 5;//calibración para convertir valor obtenido del sensor a >voltaje< del panel
  Serial.print("Voltaje del panel solar: ");
  Serial.println(voltaje);
  return voltaje;
}

String medirLDR(){
 valorLDR = analogRead(ldrPin);
 String nivelLuz ="Bajo";
  Serial.println("Valor del LDR: " + String(valorLDR));
  if (valorLDR > 4000) {
    Serial.println("Valor del LDR es mayor de 4000");
    nivelLuz = "Excelente";
  } else if (valorLDR > 3000) {
    Serial.println("Valor del LDR es mayor de 3000");
    nivelLuz = "Bueno";
  } else if (valorLDR > 2000) {
    Serial.println("Valor del LDR es mayor de 2000");
    nivelLuz = "Regular";
  } else if (valorLDR <= 1000) {
    Serial.println("Valor del LDR es menor o igual a 1000");
    nivelLuz = "Bajo";
  }
  return nivelLuz;
}


void CargarMediciones(){
  String URL = "http://192.168.0.11/SUNPASS_SO/subirMediciones.php";
  float voltajePanel = medirTension();
  String nivelLuz = medirLDR();
  String cargaMediciones = "voltaje=" + String(voltajePanel) + "&ldr=" + nivelLuz + "&ubicacion_id=" + String(ubicacionID) + "&hora=" + hora + "&dia=" + dia ;

  HTTPClient http;
  http.begin(URL);

  String payload = http.getString();
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Insertando Datos...");
  lcd.setCursor(0,1);
  lcd.print("estado: "+payload);
  lcd.setCursor(0,2);
  lcd.print("Conexion: "+estado_conexion);
}

#include <HTTPClient.h>

void ObtenerUsuariosDB() {
  String URL = "http://192.168.0.11/SUNPASS_SO/obtenerUsuarios.php";

  HTTPClient http;
  http.begin(URL);

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    // Parse JSON data
    DynamicJsonDocument doc(1024); // Adjust the size based on your JSON data
    deserializeJson(doc, payload);

    // Check if the entered user ID is valid
    bool validUserId = false;
    Serial.println("---------------- USUARIOS ----------------");

    // Extract and print specific fields
    for (JsonVariant user : doc.as<JsonArray>()) {
      int userId = user["id"];
      const char* userName = user["name"];
      const char* userEmail = user["email"];

      // Print the extracted fields
      Serial.print("ID: ");
      Serial.println(userId);
      Serial.print("Nombre: ");
      Serial.println(userName);
      Serial.print("Email: ");
      Serial.println(userEmail);
      Serial.println("------");

      preferences.putString("nombreUsuario", String(userName));
    }
  Serial.println(">>>> Ingrese el ID del usuario de la nueva ubicación:");
  while (!Serial.available()) {
    // Wait for user input
  }
  usuario_id = Serial.parseInt(); // Read the entered integer

  Serial.print("ID ingresado: ");
  Serial.println(usuario_id);
  Serial.println("Mostrando sus ubicaciones...");

  ObtenerUbicacionesPorUsuario();

  } else {
    Serial.println("Error on HTTP request");
  }

  http.end(); // Free resources
}

void ObtenerUbicacionesPorUsuario() {
  String URL = "http://192.168.0.11/SUNPASS_SO/obtenerDireccionPorUsuario.php";

  // Add the user ID as a parameter in the URL
  URL += "?usuario_id=" + String(usuario_id);

  HTTPClient http;
  http.begin(URL);

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    // Parse JSON data
    DynamicJsonDocument doc(1024); // Adjust the size based on your JSON data
    deserializeJson(doc, payload);

    Serial.println("---------------- UBICACIONES de "+ preferences.getString("nombreUsuario","Usuario") +" ----------------");
    // Extract and print specific fields
    for (JsonVariant location : doc.as<JsonArray>()) {
      int ubicacionID = location["ubicacion_id"];
      const char* ubicacionNom = location["nombre"];
      latitud = String(location["latitud"].as<float>(), 6);
      longitud = String(location["longitud"].as<float>(), 6);

      int diasDocumentados = location["dias_documentados"];

      // Print the extracted fields
      Serial.print("ID ubicacion: ");
      Serial.println(ubicacionID);
      Serial.print("Nombre Ubicación: ");
      Serial.println(ubicacionNom);
      Serial.print("Latitud: ");
      Serial.println(latitud);
      Serial.print("Longitud: ");
      Serial.println(longitud);
      Serial.print("Dias Documentos: ");
      Serial.println(diasDocumentados);
      Serial.println("------");
      }

      //Aqui quiero que solicite ingresar un id de ubicacion
      
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end(); // Free resources
}

void CambiarUbicacion() {
  ObtenerUsuariosDB();
}

