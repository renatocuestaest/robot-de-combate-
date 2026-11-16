#include <WiFi.h>
#include <WebServer.h>

// Credenciales de la red WiFi
const char* ssid = "renatzz";
const char* password = "password123";

WebServer server(80);

// Asignación de Pines
const int M_IZQ_A = 16; // Relé 1
const int M_IZQ_B = 17; // Relé 2
const int M_DER_A = 26; // Relé 3
const int M_DER_B = 27; // Relé 4
const int MOTOR_30A = 32; // Relé 30A

#define RELAY_ON LOW
#define RELAY_OFF HIGH

// INTERFAZ WEB 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    body { 
      text-align: center; font-family: Arial, sans-serif; background-color: #222; color: white; margin: 0; padding-top: 30px; 
      /* Prevenir selección de texto y lupa en celulares al mantener presionado */
      -webkit-user-select: none; user-select: none; -webkit-touch-callout: none;
    }
    /* Diseño tipo Joystick (Cuadrícula 3x3) */
    .d-pad { 
      display: grid; grid-template-columns: repeat(3, 100px); grid-gap: 10px; 
      justify-content: center; margin-bottom: 40px; 
    }
    .btn { 
      width: 100px; height: 100px; font-size: 55px; border-radius: 20px; border: none; color: white; cursor: pointer; 
      outline: none; touch-action: manipulation; -webkit-tap-highlight-color: transparent; 
    }
    .btn-dir { background-color: #4CAF50; box-shadow: 0 8px #2e7d32; }
    .btn-dir:active { background-color: #45a049; box-shadow: 0 4px #2e7d32; transform: translateY(4px); }
    
    .btn-stop { background-color: #f44336; font-size: 26px; font-weight: bold; box-shadow: 0 8px #c62828; }
    .btn-stop:active { background-color: #d32f2f; box-shadow: 0 4px #c62828; transform: translateY(4px); }
    
    /* Botón del Motor 30A */
    .btn-arma { 
      background-color: #ff9800; width: 320px; height: 80px; font-size: 28px; font-weight: bold; 
      border-radius: 20px; border: none; color: white; box-shadow: 0 8px #e65100; transition: 0.2s; 
    }
    .btn-arma:active { transform: translateY(4px); box-shadow: 0 4px #e65100; }
    .arma-on { background-color: #d84315; border: 4px solid white; box-shadow: 0 8px #bf360c; }
    
    .empty { visibility: hidden; }
  </style>
</head>
<body oncontextmenu="return false;">
  <h2>ROBOT DE COMBATE</h2>
  
  <div class="d-pad">
    <div class="empty"></div>
    <button class="btn btn-dir" onmousedown="snd('/forward')" onmouseup="snd('/stop')" ontouchstart="snd('/forward')" ontouchend="snd('/stop')">&#8679;</button>
    <div class="empty"></div>
    
    <button class="btn btn-dir" onmousedown="snd('/left')" onmouseup="snd('/stop')" ontouchstart="snd('/left')" ontouchend="snd('/stop')">&#8678;</button>
    <button class="btn btn-stop" onmousedown="globalStop()" ontouchstart="globalStop()">STOP</button>
    <button class="btn btn-dir" onmousedown="snd('/right')" onmouseup="snd('/stop')" ontouchstart="snd('/right')" ontouchend="snd('/stop')">&#8680;</button>
    
    <div class="empty"></div>
    <button class="btn btn-dir" onmousedown="snd('/backward')" onmouseup="snd('/stop')" ontouchstart="snd('/backward')" ontouchend="snd('/stop')">&#8681;</button>
    <div class="empty"></div>
  </div>

  <button id="btnArma" class="btn btn-arma" onclick="toggleArma()">MOTOR 30A: OFF</button>

  <script>
    let armaState = false; // Memoria para saber si el motor grande está prendido o apagado
    
    // Función que envía la señal al ESP32 sin recargar la página
    function snd(url) { 
      fetch(url).catch(e => console.log(e)); 
    }
    
    // Función de encendido/apagado alternado para el motor de 30A
    function toggleArma() {
      armaState = !armaState; // Invierte el estado actual
      let b = document.getElementById('btnArma');
      if(armaState) {
        b.innerText = 'MOTOR 30A: ON';
        b.classList.add('arma-on');
        snd('/motor3_on');
      } else {
        b.innerText = 'MOTOR 30A: OFF';
        b.classList.remove('arma-on');
        snd('/motor3_off');
      }
    }
    
    // Parada de emergencia Total (Ruedas + Motor 30A)
    function globalStop() {
       snd('/stop_all');
       armaState = false;
       let b = document.getElementById('btnArma');
       b.innerText = 'MOTOR 30A: OFF';
       b.classList.remove('arma-on');
    }

    // Desactivar completamente el menú de "click derecho" en celulares para no interrumpir el manejo
    window.oncontextmenu = function(event) {
        event.preventDefault();
        event.stopPropagation();
        return false;
    };
  </script>
</body>
</html>
)rawliteral";




void stopWheels() {
  digitalWrite(M_IZQ_A, RELAY_OFF);
  digitalWrite(M_IZQ_B, RELAY_OFF);
  digitalWrite(M_DER_A, RELAY_OFF);
  digitalWrite(M_DER_B, RELAY_OFF);
}

void setup() {
  Serial.begin(115200);

  // Configurar pines
  pinMode(M_IZQ_A, OUTPUT); pinMode(M_IZQ_B, OUTPUT);
  pinMode(M_DER_A, OUTPUT); pinMode(M_DER_B, OUTPUT);
  pinMode(MOTOR_30A, OUTPUT);
  
  // Asegurar que todo inicie apagado
  stopWheels();
  digitalWrite(MOTOR_30A, RELAY_OFF);

  // Iniciar Punto de Acceso (AP)
  Serial.println("Iniciando AP WiFi...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Direccion IP del Robot: ");
  Serial.println(IP); 

  // ==== RUTAS DEL SERVIDOR ====
  
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/forward", []() {
    digitalWrite(M_IZQ_A, RELAY_ON); digitalWrite(M_IZQ_B, RELAY_OFF);
    digitalWrite(M_DER_A, RELAY_ON); digitalWrite(M_DER_B, RELAY_OFF);
    server.send(200, "text/plain", "Adelante");
  });

  server.on("/backward", []() {
    digitalWrite(M_IZQ_A, RELAY_OFF); digitalWrite(M_IZQ_B, RELAY_ON);
    digitalWrite(M_DER_A, RELAY_OFF); digitalWrite(M_DER_B, RELAY_ON);
    server.send(200, "text/plain", "Atras");
  });

  server.on("/left", []() {
    digitalWrite(M_IZQ_A, RELAY_OFF); digitalWrite(M_IZQ_B, RELAY_ON); 
    digitalWrite(M_DER_A, RELAY_ON); digitalWrite(M_DER_B, RELAY_OFF); 
    server.send(200, "text/plain", "Izquierda");
  });

  server.on("/right", []() {
    digitalWrite(M_IZQ_A, RELAY_ON); digitalWrite(M_IZQ_B, RELAY_OFF); 
    digitalWrite(M_DER_A, RELAY_OFF); digitalWrite(M_DER_B, RELAY_ON); 
    server.send(200, "text/plain", "Derecha");
  });

  // El "/stop" normal se activa al soltar una flecha direccional (solo para ruedas)
  server.on("/stop", []() {
    stopWheels();
    server.send(200, "text/plain", "Ruedas Stop");
  });

  // El "/stop_all" se activa al aplastar el botón rojo STOP (Detiene TODO)
  server.on("/stop_all", []() {
    stopWheels();
    digitalWrite(MOTOR_30A, RELAY_OFF);
    server.send(200, "text/plain", "Stop Total");
  });

  server.on("/motor3_on", []() {
    digitalWrite(MOTOR_30A, RELAY_ON);
    server.send(200, "text/plain", "Motor 3 ON");
  });

  server.on("/motor3_off", []() {
    digitalWrite(MOTOR_30A, RELAY_OFF);
    server.send(200, "text/plain", "Motor 3 OFF");
  });

  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();
}