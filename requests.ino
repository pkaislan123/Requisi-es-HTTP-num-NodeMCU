// ############# LIBRARIES ############### //
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>

const char* SSID = "GNET_FIBRA_AISLAN"; // rede wifi
const char* PASSWORD = "999416698"; // senha da rede wifi

String id_central = "1";
//String BASE_URL = "http://192.168.100.8:8080/";
String BASE_URL = "https://pkaislan234-40114.portmap.io:40114/";
String path_sensoriamento = "v1/sensoriamento/inserir_registro";
String path_handle_acao = "v1/registroacao/listarAcoesNaoRespondidasPorCentral/";
String path_responder_requisicao = "v1/registroacao/responderRequisicao/";
String path_responder_requisicao_status_reles = "v1/registroacao/responderRequisicaoStatusReles/";



WiFiClientSecure  wifiClient;


SoftwareSerial mySerial(D1, D0); // RX, TX


// ############# PROTOTYPES ############### //

void initSerial();
void initWiFi();
void httpRequest(String path);

HTTPClient http;
WiFiUDP udp;



unsigned long intervalo_handle_acao = 500;
unsigned long tempo_decorrido_handle_acao = millis();

boolean requisicoes_recebidas = false;


void setup() {

  Serial.begin(115200);
  mySerial.begin(38400);

  initWiFi();

  wifiClient.setInsecure();
  //  wifiClient.connect(BASE_URL, 443);



  Serial.println("Iniciado...");
}

// ############### LOOP ################# //

void loop() {


  //ler valores do arduino

  if (mySerial.available()) {

    String recebido = mySerial.readString();
    Serial.print("Dado recebido do arduino: ");
    Serial.println(recebido);

    int tipo = recebido.indexOf("R");

    if (tipo > -1) {
      Serial.print("Responder requisicao de acao em rele");

      //requisicao
      int arroba = recebido.indexOf("@");
      String id_requisicao = recebido.substring(tipo + 1, arroba);

      Serial.print("Id da requisicao a responder: ");
      Serial.println(id_requisicao);

      responderRequisicao(id_requisicao);

    }

    tipo = recebido.indexOf("ST");
    if (tipo > -1) {
      Serial.println("Responder requisicao de status de reles");

      int modal =  recebido.indexOf("%");
      String resposta = recebido.substring(tipo + 2, modal);

      Serial.print("Resposta: ");
      Serial.println(resposta);

      responderRequisicaoStatusReles(resposta);

    }

    tipo = recebido.indexOf("SE");
    if (tipo > -1) {
      Serial.println("Responder requisicao de sensoriamento");

      int hashtag = recebido.indexOf("#");
      int arroba = recebido.indexOf("@");
      


      String id_sensor = recebido.substring(tipo + 2, hashtag);
      String valor  = recebido.substring(hashtag + 1,  arroba);

      Serial.print("Valor do id: ");
      Serial.println(id_sensor);
      Serial.print("Valor: ");
      Serial.println(valor);

      registrar_sensoriamento(id_sensor, valor);
    }

  }

  if (!requisicoes_recebidas) {
    if (millis () - tempo_decorrido_handle_acao >= intervalo_handle_acao ) {
      handleRele();
      tempo_decorrido_handle_acao = millis();
    }
  } else {
    if (millis () - tempo_decorrido_handle_acao >= (intervalo_handle_acao + 2000)) {
      requisicoes_recebidas = false;
      tempo_decorrido_handle_acao = millis();
    }

  }


}

void initWiFi() {
  delay(1000);
  Serial.println("Conectando-se em: " + String(SSID));

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado na Rede " + String(SSID) + " | IP => ");
  Serial.println(WiFi.localIP());
  delay(1000);

}

String registrar_sensoriamento(String id, String valor) {
  http.begin(wifiClient, BASE_URL + path_sensoriamento);
  http.addHeader("content-type", "application/json");
  //String body = "{ \"id_cliente\": \"" + id_cliente_ativo + "\",\"id_cartao\":" + id_cartao + "\",       }";


  String body = "{\"id_sensor\":\"" + id +  "\", \"valor\":\"" + valor + "\"}";


  int httpCode = http.POST(body);
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "request error - " + httpCode;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("request error - " + httpCode);
    return "";
  }
  String response =  http.getString();
  http.end();

  return response;
}

String responderRequisicao(String id) {
  http.begin(wifiClient, BASE_URL + path_responder_requisicao + id);
  http.addHeader("content-type", "application/json");

  int httpCode = http.GET();
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "request error - " + httpCode;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("request error - " + httpCode);
    return "";
  }
  String response =  http.getString();
  http.end();

  Serial.println(response);

  return response;
}


String responderRequisicaoStatusReles(String resposta) {
  http.begin(wifiClient, BASE_URL + path_responder_requisicao_status_reles  + resposta);
  http.addHeader("content-type", "application/json");

  int httpCode = http.GET();
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "request error - " + httpCode;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("request error - " + httpCode);
    return "";
  }
  String response =  http.getString();
  http.end();

  Serial.println(response);

  return response;
}

String handleRele() {

  http.begin(wifiClient, BASE_URL + path_handle_acao + id_central);
  http.addHeader("content-type", "application/json");
  //http.connectSSL

  int httpCode = http.GET();
  // Serial.print("Codigo: ");
  // Serial.println(httpCode);
  if (httpCode < 0 ) {
    // Serial.print("request error: ");
    // Serial.println(httpCode);
    requisicoes_recebidas = false;
    return "";
  }

  if (httpCode == 404) {
    //   Serial.print("request error: ");
    //  Serial.println(httpCode);
    //  Serial.println("Nenhuma requisicao");
    //   requisicoes_recebidas = false;
    return "";
  }
  requisicoes_recebidas = true;

  String response =  http.getString();
  http.end();

  Serial.println(response);
  mySerial.println(response);

}
