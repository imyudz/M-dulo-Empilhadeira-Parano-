/*
  PREDEFINIÇÕES PARA OS STATUS DO PAGER
    0-LAST WILL
    1-RECEBIDO
    2-ACEITO
    3-CANCELADO
    4-PARADO
    5-FINALIZADO
    6-DESCONECTADO COM OPERAÇÃO EM ANDAMENTO
    7-PARADA FINALIZADA
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            BIBLIOTECAS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include        <Arduino.h>          //padrões arduino
#include        <Wire.h>             //Display
#include        "SSD1306.h"          //Display
#include        <PubSubClient.h>     //MQTT
#include        <WiFiClient.h>       //MQTT
#include        <ESP8266WiFi.h>      //WiFi
#include        <Preferences.h>      //EEPROM
#include        <neotimer.h>         //Substituição do Millis

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            CÓDIGO DE PARADAS                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////
String    MANUT_MEC       =    "2";
String    MANUT_EL        =    "1";
String    BANHEIRO        =    "7";
String    REFEICAO        =    "9";
String    AUDITORIA       =    "11";
String    TREINAMENTO     =    "13";
String    TRY_OUT         =    "17";
String    FALTA_ABASTEC   =    "24";
String    MANUT_PREV      =    "25";
String    TESTES_DIV      =    "36";
String    NAO_APONTADO    =    "49";
String    TESTE_MANUT     =    "59";
String    AGUARD_EMPILH   =    "76";

String cod_parada = "";

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Configuração WIFI
////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* ssid = "ParanoaSA";
const char* password = "0e6f1e0dff";
//const char* ssid = "victor";
//const char* password = "admin123";
WiFiClient wifiClient;


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Configuração MQTT
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*************************SERVER:
const char* BROKER_MQTT = "192.168.0.13"; // URL DO BROKER que se deseja utilizar, nese caso o broker da eclipse
int BROKER_PORT = 1883; //Porta do BROKER MQTT
#define MQTTsubQos  1   //qos of subscribe
#define MQTTpubQos  1   //qos of subscribe
//*************************CLIENT:
PubSubClient MQTT(wifiClient); //Instancia objeto MQTT que se vincula ao objeto do espClient
//****************DEFINE OS TÓPICOS DESSE PAGER
#define TOPIC_PUBLISH "PG1/STATUS"
const char* PAGER_NAME = "PG2";
String STATUS_PAGER = (String)PAGER_NAME+"/STATUS";
String UNIDADE = (String)PAGER_NAME+"/LOCAL";
String PRODUTO = (String)PAGER_NAME+"/PRODUTO";
String SOLICITACAO = (String)PAGER_NAME+"/SOLICITACAO";
String COD_PARADA = (String)PAGER_NAME+"/CODPARADA";


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     MAPEAMENTO DE HARDWARE
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//definições para os botões da placa
unsigned long tempo_botao = millis();
#define btcima 2 //D4
#define btbaixo 12 //D3
#define btenter 0 //D6
#define buzzer 14
#define DEBOUNCE_BOTAO 300
#define QUANT_VEZES_BUZZER 4


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   CONFIGURAÇÕES DO MENU E DISPLAY
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Adafruit_SSD1306 display(128, 64, &Wire, -1);
SSD1306Wire display(0x3c, SDA, SCL);
//variaveis menu
byte navegacao_status = 0; //controla em qual página do MENU o dispositivo se encontra durante a navegação
byte altera_parametros = 0; //indica o parâmtro que está sendo atualizado
String sts_sistema = "";   //define o status da integridade dos dados do dispositivo. Se o valor for maior que 0, alguma informação não foi cadastrada no dispositivo e por isso não acontecerá a comunicação com o servidor MQTT
String produtoDisplay; //recebe o produto que foi atribuido


////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Configurações EEPROM                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EEPROM_SOLICITACOES "blocoSolicitacoes"
#define EEPROM_ENDERECO_SOLICITACOES "id_solicitacoes"
Preferences EEPROMdata;
float id_solicitacoes;

////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Configurações NEOTIMER                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////
Neotimer tempoBuzzerLigado = Neotimer(70);
Neotimer tempoBuzzerDesligado = Neotimer(70);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           TAGS GLOBAIS                                                    //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
String status_chamada = "";
String produto = "";
String local = "";
String solicitacao = "";
int selecionado = 0;
int navegado = -1;
int selecionadoOP = 0;
int navegadoOP = -1;
int selecionadoParada = 0;
int navegadoParada = -1;
int selecionadoFinalParada = 0;
int navegadoFinalParada = -1;
unsigned long ultimoEnvio = 0;
unsigned long ultimoEnvio2 = 0;
unsigned long ultimoEnvio3 = 0;
unsigned long ultimoEnvio4 = 0;
unsigned long ultimoEnvio5 = 0;
unsigned long ultimoEnvio6 = 0;
unsigned long ultimoEnvio7 = 0;
unsigned long ultimaChamada = 0;
unsigned long tempoToque = 0;
int salva_ponto;
int estado_antes_parada = 0;
boolean desconexaoPendente = false;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Endereços de Imagens 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const unsigned char PROGMEM logo_dw [] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x03, 0x80, 0x00, 0x00, 0x0F, 0xF0, 0x03, 0x80, 0x00, 0x00, 0x0F, 0xF8, 0x07, 0xC0, 0x00, 0x00, 0x70, 0x3C, 0x07, 0xC0, 0x00, 0x00, 0x70, 0x1E, 0x06, 0xE0, 0x00, 0x00, 0x70, 0x0E, 0x0E, 0xE0, 0x00, 0x00, 0x70, 0x07, 0x0C, 0x70, 0x00, 0x00, 0x70, 0x07, 0x1C, 0x70, 0x00, 0x00, 0x70, 0x07, 0x1C, 0x38, 0x00, 0x00, 0x70, 0x07, 0x38, 0x38, 0x00, 0x00, 0x70, 0x07, 0x38, 0x38, 0x00, 0x00, 0x70, 0x0E, 0x70, 0x1C, 0x00, 0x00, 0x70, 0x1E, 0x70, 0x1C, 0x00, 0x00, 0x70, 0x7C, 0x60, 0x0E, 0x00, 0x00, 0x7F, 0xF8, 0xE0, 0x0E, 0x00, 0x00, 0x7F, 0xF0, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0xC0, 0x07, 0x00, 0x00, 0x38, 0x01, 0xE0, 0x0E, 0x00, 0x00, 0x18, 0x03, 0xE0, 0x0E, 0x00, 0x00, 0x1C, 0x03, 0xF0, 0x0C, 0x00, 0x00, 0x1C, 0x07, 0x70, 0x1C, 0x00, 0x00, 0x0E, 0x07, 0x30, 0x18, 0x00, 0x00, 0x0E, 0x0E, 0x38, 0x38, 0x00, 0x00, 0x06, 0x0E, 0x38, 0x38, 0x00, 0x00, 0x07, 0x0C, 0x1C, 0x70, 0x00, 0x00, 0x03, 0x1C, 0x1C, 0x70, 0x00, 0x00, 0x03, 0x9C, 0x0E, 0x60, 0x00, 0x00, 0x03, 0xB8, 0x0E, 0xE0, 0x00, 0x00, 0x01, 0xF8, 0x06, 0xC0, 0x00, 0x00, 0x01, 0xF0, 0x07, 0xC0, 0x00, 0x00, 0x00, 0xF0, 0x07, 0xC0, 0x00, 0x00, 0x00, 0xE0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
/*const unsigned char PROGMEM icone_config [] = {0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x0E, 0x70, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0x7C, 0x3E, 0x00, 0x00, 0x38, 0x1C, 0x00, 0x00, 0x38, 0x1C, 0x00, 0x00, 0x7C, 0x3E, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x70, 0x7F, 0xFE, 0x00, 0x70, 0x0F, 0xF0, 0x00, 0x70, 0x0F, 0xF0, 0x1C, 0xF9, 0xCE, 0x70, 0x3F, 0xFF, 0xE4, 0x20, 0x1F, 0xFF, 0xC0, 0x00, 0x0F, 0xFF, 0x80, 0x00, 0x1F, 0x8F, 0x80, 0x00, 0x1F, 0x07, 0xC0, 0x00, 0xFE, 0x03, 0xF8, 0x00, 0xFE, 0x03, 0xF8, 0x00, 0xFE, 0x03, 0xF8, 0x00, 0x1E, 0x07, 0xC0, 0x00, 0x1F, 0x07, 0x80, 0x00, 0x0F, 0xFF, 0x80, 0x00, 0x1F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x1D, 0xFD, 0xC0, 0x00, 0x08, 0x70, 0x80, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00};*/
/*const unsigned char PROGMEM icone_info [] = {0x7F, 0xFF, 0xFF, 0xF0, 0xC0, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x8E, 0x7F, 0xFF, 0x10, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x8E, 0x7F, 0xFF, 0x10, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x8E, 0x7F, 0xFF, 0x10, 0x80, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0x86, 0x7F, 0xC0, 0x10, 0x80, 0x00, 0x03, 0x00, 0x80, 0x00, 0x1F, 0xE0, 0x80, 0x00, 0x33, 0x38, 0x80, 0x00, 0xCF, 0xCC, 0x80, 0x01, 0x88, 0x46, 0xFF, 0xFB, 0x1B, 0x63, 0x00, 0x03, 0x1B, 0x23, 0x00, 0x01, 0x98, 0x66, 0x00, 0x00, 0xCC, 0xCC, 0x00, 0x00, 0x67, 0x98, 0x00, 0x00, 0x3C, 0xF0, 0x00, 0x00, 0x0F, 0xC0};*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         Códigos das Funções                                              //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void (*funcReset)() = 0;

void apitaBuzzer(){
  for (int i = 0; i < QUANT_VEZES_BUZZER; i++)
  {
    //LÓGICA EM TESTE:
    /*  tempoBuzzerLigado.start();
      if(tempoBuzzerLigado.waiting()) digitalWrite(buzzer, HIGH);
      if(tempoBuzzerLigado.done()){ digitalWrite(buzzer, LOW); tempoBuzzerDesligado.start();}
      if(tempoBuzzerDesligado.waiting()) digitalWrite(buzzer, LOW);
      if(tempoBuzzerDesligado.done()) tempoBuzzerLigado.reset();
      tempoBuzzerDesligado.reset();
    */
    digitalWrite(buzzer, HIGH);
    delay(60);
    digitalWrite(buzzer, LOW);
    delay(60);
  }
}


void menu_seleciona(){//MENU PARA SELECIONAR A AÇÃO QUANDO APARECE UMA CHAMADA
  int y_pos = 22;
  const char *opcoes[3] = {
    " ACEITAR",
    " VOLTAR",
    " PARADA"
  };

  if (navegado == -1) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.setColor(WHITE);
    display.drawString(0, 0,"ESCOLHA UMA OPÇÃO");
    for (int i = 0; i < 3; i++) {
      if (i == selecionado) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        //Serial.println(opcoes[i]);
        if (y_pos < 50){
          display.drawString(20, y_pos, ">");
          display.drawString(27, y_pos, opcoes[i]);
          y_pos += 15;
        }
        else
        {
          display.setColor(BLACK);
          y_pos = 22;
          display.fillRect(20, y_pos, 160, 50);
          display.setColor(WHITE);
          display.drawString(20, y_pos, ">");
          display.drawString(27, y_pos, opcoes[i]);
        }
      } else if (i != selecionado) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        display.setColor(WHITE);
        if (y_pos < 50){
          display.drawString(22, y_pos, opcoes[i]);
          y_pos+= 15;
        }
        else
        {
          display.setBrightness(255);
          display.setContrast(255);
          display.setFont(ArialMT_Plain_10);
          display.drawString(30, y_pos, opcoes[i]);
        }
      }
    }
  }
  display.display();
}

void menu_selecionaEmOperacao(){ //MENU PARA SELECIONAR A AÇÃO ENQUANTO ESTIVER EM OPERAÇÃO (STATUS = 2)
  
  int y_pos = 20;
  const char *opcoes[4] = {
    " FINALIZAR",
    " VOLTAR",
    " CANCELAR",
    " PARADA"
  };
  if (navegadoOP == -1) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.setColor(WHITE);
    display.drawString(0, 0,"ESCOLHA UMA OPÇÃO");
    for (int i = 0; i < 4; i++) {
      if (i == selecionadoOP) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        if (y_pos < 50){
          display.drawString(20, y_pos, ">");
          display.drawString(27, y_pos, opcoes[i]);
          y_pos += 15;
        }
        else
        {
          display.setColor(BLACK);
          y_pos = 16;
          display.fillRect(20, y_pos, 160, 50);
          display.setColor(WHITE);
          display.drawString(20, y_pos, ">");
          display.drawString(27, y_pos, opcoes[i]);
          y_pos+= 15;
        }
      } else if (i != selecionadoOP) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        display.setColor(WHITE);
        if (y_pos < 50){
          display.drawString(22, y_pos, opcoes[i]);
          y_pos+= 15;
        }
        else
        {
          display.setBrightness(255);
          display.setContrast(255);
          display.setFont(ArialMT_Plain_10);
          display.drawString(30, y_pos, opcoes[i]);
          y_pos += 15;
        }
      }
    }
  }
  display.display();
}

void menu_motivoParada(){ //MENU PARA SELECIONAR A AÇÃO ENQUANTO ESTIVER EM OPERAÇÃO (STATUS = 2)
  
  int y_pos = 20;
  const char *opcoes[14] = {
    "VOLTAR",   
    "MANUT EL",     
    "BANHEIRO",     
    "REFEICAO",     
    "AUDITORIA",    
    "TREINAMENTO",  
    "TRY OUT",      
    "FALTA ABASTEC",
    "MANUT PREV",   
    "TESTES DIV",   
    "OUTRO", 
    "TESTE MANUT",  
    "AGUARD EMPILH",
  };
  if (navegadoParada == -1) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.setColor(WHITE);
    display.drawString(0, 0,"ESCOLHA UMA OPÇÃO");
    for (int i = 0; i < 14; i++) {
      if (i == selecionadoParada) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        if (y_pos < 50){
          display.drawString(10, y_pos, ">");
          display.drawString(17, y_pos, opcoes[i]);
          y_pos += 15;
        }
        else
        {
          display.setColor(BLACK);
          y_pos = 16;
          display.fillRect(0, y_pos, 160, 50);
          display.setColor(WHITE);
          display.drawString(1, y_pos, ">");
          display.drawString(8, y_pos, opcoes[i]);
          y_pos+= 15;
        }
      } else if (i != selecionadoParada) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        display.setColor(WHITE);
        if (y_pos < 50){
          display.drawString(7, y_pos, opcoes[i]);
          y_pos+= 15;
        }
        else
        {
          display.setBrightness(255);
          display.setContrast(255);
          display.setFont(ArialMT_Plain_10);
          display.drawString(20 , y_pos, opcoes[i]);
          y_pos += 15;
        }
      }
    }
  }
  display.display();
}

void menu_confirmaFinalParada(){//MENU PARA CONFIRMR SE QUER OU NÃO O FINAL DA PARADA
  int y_pos = 22;
  const char *opcoes[2] = {
    " SIM",
    " NÃO"
  };

  if (navegadoFinalParada == -1) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.setColor(WHITE);
    display.drawString(0, 0,"FINALIZAR PARADA?");
    for (int i = 0; i < 2; i++) {
      if (i == selecionadoFinalParada) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        display.drawString(20, y_pos, ">");
        display.drawString(27, y_pos, opcoes[i]);
        y_pos += 20;
        Serial.println(opcoes[i]);

      } else if (i != selecionadoFinalParada) {
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        display.setColor(WHITE);
        display.drawString(22, y_pos, opcoes[i]);
        y_pos+= 20;
      }
    }
  }
  display.display();
}



void controle_menu(bool cima, bool enter, bool baixo) { //controla a navegação entre os níveis do MENU

  if (navegacao_status == 0) { //ESPERA TODAS AS CONEXÕES TERMINAREM  
  }
  else if (navegacao_status == 10) //tela de aguarde chamada
  {
    if(enter) navegacao_status = 10;
  }
  else if (navegacao_status == 11) //tela de chamada e aceitamento de chamada
  {
    if(enter) navegacao_status = 20;
    if(baixo) navegacao_status = 11;
    if(cima) navegacao_status = 11;    
  }
  else if (navegacao_status == 20) //MENU DE ACEITAMENTO DE OPERAÇÃO
  {
    if (cima && baixo) { //BOTÕES APERTADOS AO MESMO TEMPO
    };
    if (cima) {
      selecionado = selecionado + 1;
      delay(200);
      if(selecionado == 2){
        if (baixo)
        {
          selecionado = selecionado - 1;
        }
      }
      if (selecionado == 0)
      {
        if(cima)
        {
          selecionado = selecionado + 1;
        }
      }
      if(selecionado > 2){
        selecionado = 0;
      }
    };
    if (baixo) {
      selecionado = selecionado - 1;
      delay(200);
      if(selecionado < 0){
        selecionado = 0;
      }
    };
    if (enter){
      navegado = selecionado;
      //if (baixo) navegado = navegado - 1;
      if (navegado == 0)
      {
        navegacao_status = 30;
      }
      else if (navegado == 1){
        navegacao_status = 10;
      }
      else if (navegado == 2){
        estado_antes_parada = navegacao_status;
        navegacao_status = 85;
      }
    }
  }
  else if (navegacao_status == 40)
  {
    if (enter)
    { //abre o menu de escolha de finalização
      navegacao_status = 45;
      return;
    }
  }
  else if (navegacao_status == 45)
  {
    if (cima && baixo); //BOTÕES APERTADOS AO MESMO TEMPO;

    if (cima) 
    {
      selecionadoOP = selecionadoOP + 1;
      delay(200);
      if(selecionadoOP == 3)
      {
        if (baixo)
        {
          selecionadoOP = selecionadoOP - 1;
        }
      }
      if (selecionadoOP == 0)
      {
        if(cima)
        {
          selecionadoOP= selecionadoOP + 1;
        }
      }
      if(selecionadoOP > 3){
        selecionadoOP = 0;
      }
    }
    if (baixo) 
    {
      selecionadoOP = selecionadoOP - 1;
      delay(200);
      if(selecionadoOP < 3){
        selecionadoOP = 0;
      }
    }
    if (enter)
    {
      navegadoOP = selecionadoOP;
      if (baixo) navegadoOP = navegadoOP - 1;
      if (navegadoOP == 0)
      {
        navegacao_status = 55;
      }
      else if (navegadoOP == 1){
        navegacao_status = 40;
      }
      else if (navegadoOP == 2)
      {
        navegacao_status = 70;
      }
      else if (navegadoOP == 3)
      {
        estado_antes_parada = navegacao_status;
        navegacao_status = 85;
      }
    } 
  }
  else if (navegacao_status == 85)
  {
    if (enter)
    { //abre o menu de escolha de parada
      navegacao_status = 90;
      return;
    }
  }
  else if (navegacao_status == 90)//menu de parada
  {
    Serial.println(cod_parada);
    if (cima && baixo); //BOTÕES APERTADOS AO MESMO TEMPO;
    if (cima) 
    {
      selecionadoParada = selecionadoParada + 1;
      delay(200);
      if(selecionadoParada == 12)
      {
        if (baixo)
        {
          selecionadoParada = selecionadoParada - 1;
        }
      }
      if (selecionadoParada == 0)
      {
        if(cima)
        {
          selecionadoParada= selecionadoParada + 1;
        }
      }
      if(selecionadoParada > 12){
        selecionadoParada = 0;
      }
    }
    if (baixo) 
    {
      selecionadoParada = selecionadoParada - 1;
      delay(200);
      if(selecionadoParada < 12){
        selecionadoParada = 0;
      }
    }
    if (enter)
    {
      cod_parada = "";
      navegadoParada = selecionadoParada;
      if (baixo) navegadoParada = navegadoParada - 1;

      // "MANUT MEC",   
      // "MANUT EL",     
      // "BANHEIRO",     
      // "REFEICAO",     
      // "AUDITORIA",    
      // "TREINAMENTO",  
      // "TRY OUT",      
      // "FALTA ABASTEC",
      // "MANUT PREV",   
      // "TESTES DIV",   
      // "OUTRO", 
      // "TESTE MANUT",  
      // "AGUARD EMPILH",
      
      if      (navegadoParada == 0){  navegacao_status = estado_antes_parada; estado_antes_parada = 0;}
      else if (navegadoParada == 1)   cod_parada = MANUT_MEC;
      else if (navegadoParada == 2)   cod_parada = MANUT_EL;
      else if (navegadoParada == 3)   cod_parada = BANHEIRO;
      else if (navegadoParada == 4)   cod_parada = REFEICAO;
      else if (navegadoParada == 5)   cod_parada = AUDITORIA;
      else if (navegadoParada == 6)   cod_parada = TREINAMENTO;
      else if (navegadoParada == 7)   cod_parada = TRY_OUT;
      else if (navegadoParada == 8)   cod_parada = FALTA_ABASTEC;
      else if (navegadoParada == 9)   cod_parada = MANUT_PREV;
      else if (navegadoParada == 10)  cod_parada = TESTES_DIV;
      else if (navegadoParada == 11)  cod_parada = NAO_APONTADO;
      else if (navegadoParada == 12)  cod_parada = TESTE_MANUT;
      else if (navegadoParada == 13)  cod_parada = AGUARD_EMPILH;

      if(!cod_parada.equals("")){
        navegacao_status = 95;
      }
      Serial.println(cod_parada);
    }
  }
  else if (navegacao_status == 100)
  {
    if(enter) navegacao_status = 105;
  }
  else if (navegacao_status == 105)
  {
    if (cima && baixo) { //BOTÕES APERTADOS AO MESMO TEMPO
    };
    if (cima) {
      selecionadoFinalParada = selecionadoFinalParada + 1;
      delay(200);
      if(selecionadoFinalParada == 1){
        if (baixo)
        {
          selecionadoFinalParada = selecionadoFinalParada - 1;
        }
      }
      if (selecionadoFinalParada == 0)
      {
        if(cima)
        {
          selecionadoFinalParada = selecionadoFinalParada + 1;
        }
      }
      if(selecionadoFinalParada > 1){
        selecionadoFinalParada = 0;
      }
    };
    if (baixo) {
      selecionadoFinalParada = selecionadoFinalParada - 1;
      delay(200);
      if(selecionadoFinalParada < 0){
        selecionadoFinalParada = 0;
      }
    };
    if (enter){
      navegadoFinalParada = selecionadoFinalParada;
      //if (baixo) navegadoFinalParada = navegadoFinalParada - 1;
      if (navegadoFinalParada == 0)
      {
        navegacao_status = 110;
      }
      else if (navegadoFinalParada == 1){
        navegacao_status = 100;
      }
    }
  }
}

void controle_navegacao(int pin_cima, int pin_enter, int pin_baixo) {  //constamente verifica os botões e dispara a mudança no menu caso algum seja pressionado
  int leitura_cima = digitalRead(pin_cima);
  int leitura_enter = digitalRead(pin_enter);
  int leitura_baixo = digitalRead(pin_baixo);

  //Serial.println(leitura_cima);
  //Serial.println(leitura_enter);
  //Serial.println(leitura_baixo);

  bool cima = false;
  bool enter = false;
  bool baixo = false;
  if (leitura_cima == LOW and ((millis()- tempo_botao) > DEBOUNCE_BOTAO)) { //pressionou cima
    cima = true;
    //Serial.println(cima);
    tempo_botao = millis();
  }
  else if (leitura_enter == LOW and ((millis()- tempo_botao) > DEBOUNCE_BOTAO)) { //pressionou enter
    enter = true;
    //Serial.println(enter);
    tempo_botao = millis();
  }
  else if (leitura_baixo == LOW and ((millis() - tempo_botao) > DEBOUNCE_BOTAO)) { //pressionou baixo
    baixo = true;
    //Serial.println(baixo);
    tempo_botao = millis();
  }
  else {
    //nada pressionado
    //Serial.println("nada");
    return;
  }
  controle_menu(cima, enter, baixo);
}


void controle_display(String status_sistema, int estado_mqtt) { //responsável apenas por atualizar o display para mostrar as informações de acordo com o que é necessário pela navegação de menus externos
    display.clear();
    switch (navegacao_status) {
    case 0:
    //Serial.print("ESTOU NO CASE 0");
      //tela inicial, permite mostrar os erros do mqtt e do sistema
      if (!status_sistema.equals("")) {
        //Erro de parâmetros do sistema.
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(40, 1, "ERRO!");
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 20, status_sistema);
        display.display();

      }else if (estado_mqtt != 0){
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_10);
        // erro na comunicação com o broker MQTT
        display.drawString(0, 1,"FALHA DE COMUNICACAO!");
        display.display();
        switch (estado_mqtt)
        { //Preparado mas não configurado para gerar todas essas excessões.
          case -4: // TIMEOUT DE CONEXÃO
            display.drawString(0, 20, "TIMEOUT DE CONEXÃO");
            display.display();
            break;
          case -3: // CONEXÃO PERDIDA
            display.drawString(0, 20, "CONEXÃO PERDIDA");
            display.display();
            break;
          case -2: // FALHA AO TENTAR CONECTAR
            display.drawString(0, 20, "FALHA AO TENTAR CONECTAR AO BROKER");
            display.display();
            break;
          case -1: // DESCONECTADO
            display.drawString(0, 20, "SISTEMA DESCONECTADO");
            display.display();
            break;
          case 1: // PROTOCOLO RUIM
            display.drawString(0, 20, "PROTOCOLO RUIM");
            display.display();
            break;
          case 2: // FALHA COM ID DO DISPOSITIVO NO BROKER
            display.drawString(0, 20, "FALHA COM ID DO DISPOSITIVO NO BROKER");
            display.display();
            break;
          case 3: // INDISPONÍVEL
            display.drawString(0, 20, "BROKER INDISPONÍVEL");
            display.display();
            break;
          case 4: // CREDENCIAIS INVÁLIDAS
            display.drawString(0, 20, "CREDENCIAIS INVÁLIDAS");
            display.display();
            break;
          case 5: // NÃO AUTORIZADO
            display.drawString(0, 20, "ACESSO NÃO AUTORIZADO");
            display.display();
            break;
          }
      }else{
        //Serial.print("ESTOU NO CASE 0 (DATAWAKE)");
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 1, "DATAWAKE");
        display.drawFastImage(40, 17, 48, 48, logo_dw);
        display.display();
        if(millis() - ultimoEnvio4 >= 2000){
          navegacao_status = 10;
          ultimoEnvio4 = millis();
        }
      }
    break;
    case 10:
      //Serial.print("ESTOU NO CASE 10");
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "SEM OPERAÇÃO");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "AGUARDE");
      display.drawString(64, 36, "CHAMADA");
      display.display();
      if(millis() - ultimaChamada >= 5000){
        MQTT.publish(STATUS_PAGER.c_str(), "1", MQTTpubQos);
        ultimaChamada = millis();
      }
      if (!produto.equals("") && !local.equals(""))
      {
        navegacao_status = 11;
      }else{
        navegacao_status = 10;
      }
      
    break;
    case 11:
      apitaBuzzer();
      //Serial.print("ESTOU NO CASE 11");
      display.clear();        //limpa display
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0,0, "CHAMADA RECEBIDA!");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64,12, produtoDisplay);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 36, local);
      if(millis() - ultimoEnvio3 >= 1500){
        MQTT.publish(STATUS_PAGER.c_str(), "1");
        ultimoEnvio3 = millis();
      }
      if (produto.equals("") or local.equals(""))
      {
        navegacao_status = 10;
      }
      display.display();
    break;
    case 20:
      navegado = -1;
      display.clear();
      menu_seleciona();
    break;
    case 30:
    {
      //////////////////////*******CONFIG ENVIA MQTT*****////////////////////////////
      //Após a aceitação do pedido, atualiza as informações da chamada no driver
      MQTT.publish(STATUS_PAGER.c_str(), "2");
      
      if(status_chamada.equals("2")){
        navegacao_status = 40;
      }
      else
      {
        navegacao_status = 30;
      }
      Serial.println("fIM DO 30");      
    }
    break;
    case 40:
      //////////////////////*******CONFIG Display*****////////////////////////////
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "EM OPERAÇÃO");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "BUSQUE");
      display.drawString(64, 36, "PRODUTO");
      display.display();
      //USANDO DELAY, MAS ADAPTAR PARA BIBLIOTECA NEOTIMER
    break;
    case 45:
      navegadoOP = -1;
      display.clear();
      menu_selecionaEmOperacao();
    break;
    case 55:
    {
      //////////////////////*******CONFIG ENVIA MQTT*****////////////////////////////
      //Após a FINALIZAÇÃO do pedido, atualiza as informações da chamada no driver
      if(millis() - ultimoEnvio >= 1500){
        MQTT.publish(STATUS_PAGER.c_str(), "5");
        ultimoEnvio = millis();
      }      
      if(status_chamada.equals("5")){
        navegacao_status = 60;
      }
    }
    break;
    case 60:
      //Antes de Tudo, limpa o id da EEPROM e confirma se limpou
      EEPROMdata.begin(EEPROM_SOLICITACOES);
      EEPROMdata.putFloat(EEPROM_ENDERECO_SOLICITACOES, 0);
      Serial.println(" Conteúdo da EEPROM após finalizar operação: " + (int)EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0));
      EEPROMdata.end();
      //Desenha no Display
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Parabéns!");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "Operação");
      display.drawString(64, 36, "FINALIZADA");
      display.display();
      //MQTT.publish(STATUS_PAGER.c_str(), "1");
      if(status_chamada.equals("1")){
        navegacao_status = 10;
      }else{
        //delay(500);
        //navegacao_status = 10;
      }
    break;
    case 70:
    {
      if(millis() - ultimoEnvio2 >= 1500){
        MQTT.publish(STATUS_PAGER.c_str(), "3");
        ultimoEnvio2 = millis();
      }
      if (status_chamada.equals("3"))
      {
        navegacao_status = 75;
      }
    }
    break;
    case 75:
      //Antes de Tudo, limpa o id da EEPROM e confirma se limpou
      EEPROMdata.begin(EEPROM_SOLICITACOES);
      EEPROMdata.putFloat(EEPROM_ENDERECO_SOLICITACOES, 0);
      Serial.println(" Conteúdo da EEPROM após cancelar operação: " + (int)EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0));
      EEPROMdata.end();
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Aguarde...");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "Operação");
      display.drawString(64, 36, "CANCELADA");
      display.display();
      if (status_chamada.equals("1"))
      {
        navegacao_status = 10;
      } else {
        delay(500);
        navegacao_status = 10;
      }
    break;
    case 80:
      EEPROMdata.begin(EEPROM_SOLICITACOES);
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Aguarde...");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "AGUARDO");
      display.drawString(64, 36, "SISTEMA");
      display.display();
      if(status_chamada.equals("2")){
        if (millis() - ultimoEnvio5 >= 3000)
        {
          ultimoEnvio5 = millis();
          navegacao_status = 40;
        }
      }
      else if (status_chamada.equals("4"))
      {
        navegacao_status = 100;
      }
      else if(EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0) == 0)
      {
        Serial.println("RECEBI 0, VOLTANDO PARA 10");
        if (millis() - ultimoEnvio5 >= 3000)
        {
          navegacao_status = 10;
          ultimoEnvio5 = millis();
        }
      }
      else
      {
        int flag = 1;
        if(flag == 1){ // Só para não ler a memória toda hora
          Serial.println("AGUARDO DO MQTT --> DADO NA MEMÓRIA: " + (String)EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0));
          flag = 2;
        }
        funcReset();
      }
      EEPROMdata.end();
    break;
    case 85:
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "APERTE ENTER");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "ESCOLHA");
      display.drawString(64, 36, "MOTIVO");
      display.display();
    break;
    case 90:
      navegadoParada = -1;
      display.clear();
      menu_motivoParada();
    break;
    case 95:
      if(millis() - ultimoEnvio6 >= 1500){
        MQTT.publish(COD_PARADA.c_str(), cod_parada.c_str());
        Serial.println(cod_parada.c_str());
        MQTT.publish(STATUS_PAGER.c_str(), "4");
        ultimoEnvio6 = millis();
      }
      if (status_chamada.equals("4"))
      {
        navegacao_status = 100;
      }
    break;
    case 100:
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "EMPILHADEIRA PARADA");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "ENTER P/");
      display.drawString(64, 36, "FINALIZAR");
      display.display();
    break;
    case 105:
      navegadoFinalParada = -1;
      display.clear();
      menu_confirmaFinalParada();
    break;
    case 110:
      if(millis() - ultimoEnvio7 >= 1500){
        MQTT.publish(COD_PARADA.c_str(), "0");
        MQTT.publish(STATUS_PAGER.c_str(), "7");
        ultimoEnvio7 = millis();
      }
      if (status_chamada.equals("7"))
      {
        navegacao_status = 10;
      }
    break;
    default:
      Serial.print("Default");
      display.clear();
    break;
  }
  display.display();
}


void conectaWiFi(){

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  
  WiFi.begin(ssid, password); // Conecta na rede WI-FI
  
  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Buscando...");

    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 12, "Conectando");
    display.drawString(64, 36, "WiFi");    
    display.display();    
  }

    Serial.println();
    Serial.print("Conectado com sucesso, na rede: ");
    Serial.print(ssid);  
    Serial.print("  IP obtido: ");
    Serial.println(WiFi.localIP()); 
    
    display.clear();        //limpa display
    display.display();      //efeuta operações
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Sucesso!");
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 12, "WiFi");
    display.drawString(64, 36, "Conectado"); 
    display.display();
    delay(4000);   
}

void connectMQTT() {

// Loop until we're reconnected
  while (!MQTT.connected()) {
    
    Serial.print("Attempting MQTT connection...");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Aguarde...");
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 12, "Conectando");
    display.drawString(64, 36, "Fábrica");
    display.display();
    
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect

    //(const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage)
    if (MQTT.connect(clientId.c_str(), STATUS_PAGER.c_str(), 1, true, "0")) { //define um id aleatório, poderia ser fixo, se definido lá em cima
      Serial.println("connected");
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Sucesso!");
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 12, "Fábrica");
      display.drawString(64, 36, "Conectada");
      display.display();
      //delay(1000);
      //Após conectar faz subscribe em todos os tópicos necessários
      MQTT.subscribe(UNIDADE.c_str(), MQTTsubQos);
      MQTT.subscribe(PRODUTO.c_str(), MQTTsubQos);
      MQTT.subscribe(STATUS_PAGER.c_str(), MQTTsubQos);
      MQTT.subscribe(SOLICITACAO.c_str(), MQTTsubQos);
      //MQTT.subscribe(COD_PARADA.c_str(), MQTTsubQos);
      delay(500);
      //Uma vez conectado, publica que está disponível para o ELIPSE
      MQTT.publish(STATUS_PAGER.c_str(), "1");
      // ... and resubscribe
      navegacao_status = 0;
    } else {
      Serial.print("failed, rc=");
      Serial.print(MQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying 
      delay(5000);
    }
  }
  
}


void callback(char* topic, byte* payload, unsigned int length) {
  //Tratamento de Tópicos Recebidos
    String topico = topic;
    Serial.print("Mensagem recebida: [");
    Serial.print(topico);
    Serial.println("] ");
    
    if(!topico.equals("")){
      if (topico.equals(STATUS_PAGER))
      {
        status_chamada = "";
        for (int i = 0; i < length; i++) {                                    
          Serial.print((char)payload[i]);
          Serial.println((char)payload[i]);
          status_chamada = status_chamada + (char)+payload[i];
        }
        if(status_chamada.equals("1")){
          status_chamada = "1";
        }
        else if(status_chamada.equals("2"))
        {
          status_chamada = "2";
        }
        else if (status_chamada.equals("6"))
        {
          status_chamada = "6";
          navegacao_status = 80;
        }
        Serial.println();
        Serial.println("STATUS_PAGER: "+status_chamada);
        
      }
      if (topico.equals(UNIDADE))
      {
        local = "";
        status_chamada = "1";
        for (int i = 0; i < length; i++) {                                    
          Serial.print((char)payload[i]);
          local = local + (char)payload[i];
        }
        Serial.println();
        Serial.println("Unidade: "+local);
      }
      if(topico.equals(PRODUTO))
      {
        produto = "";
        status_chamada = "1";
        for (int i = 0; i < length; i++) {    
          Serial.print((char)payload[i]);
          produto = produto + (char)+payload[i];
        }
        
        Serial.println("Produto: "+produto);
        if (produto.equals("EMBALAGEM"))
        {
          produto = "EMBLG";
          Serial.println("EMBLG");
        }
        produtoDisplay = produto;
        Serial.println(produto);
      }
      if (topico.equals(SOLICITACAO))
      {
        solicitacao = "";
        for(int i = 0; i < length; i++){
          Serial.print((char)payload[i]);
          solicitacao = solicitacao + (char)+payload[i];
        }
        Serial.println("\nID_SOLICITAÇÃO: " +solicitacao);
        id_solicitacoes = solicitacao.toFloat();
        Serial.println(id_solicitacoes);
        //TODA VEZ QUE RECEBER ALGUM DADO DO ID_SOLICITAÇÃO, GRAVA ELE NA EEPROM:
        EEPROMdata.begin(EEPROM_SOLICITACOES);
        Serial.println("O que tem antes da escrita na memória: " + (String)EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0));   //Verifica o que há na memória antes de escrever
        if(EEPROMdata.putFloat(EEPROM_ENDERECO_SOLICITACOES, id_solicitacoes) == 0)
        {
            Serial.println("ERRO NA ESCRITA");   //Verifica o que há na memória antes de escrever
        }
        Serial.println("O que tem depois da escrita na memória: " + (String)EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0));  //Verifica se realmente escreveu
        EEPROMdata.end();
      }
    }else{
      navegacao_status = 10;
      status_chamada = "";
    }
}


void processaDesconexoes() {
  if (WiFi.status() == WL_CONNECTED) {
    if (MQTT.connected()) {
      MQTT.loop();
    } else {
      navegacao_status = 0;
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
      // client id, client username, client password, last will topic, last will qos, last will retain, last will message
      if (MQTT.connect(clientId.c_str(), STATUS_PAGER.c_str(), 1, true, "0")) {
          desconexaoPendente = false;
          Serial.println("Reconectado ao MQTT");
          navegacao_status = salva_ponto;
      }
    }
  } else {
    //quando há uma desconexão, salva o status parado, se ele já for a partir de 30. Manda navegação status para 0.
    sts_sistema = "WiFi voltando.......";
    
    Serial.println("Desconectado do WiFi");
    if (MQTT.connected())
      Serial.println("Desconectando MQTT...");
      MQTT.disconnect();
      if (navegacao_status >= 30)
      {
        salva_ponto = navegacao_status;
      }else{
        salva_ponto = 10;
      }
        navegacao_status = 0;
  }
  if (!MQTT.connected() && !desconexaoPendente) {
    desconexaoPendente = true;
    Serial.println("MQTT desconectado com sucesso!");
  }
}


void verificaMemoria(float solicitacao){ //Toda vez que ligar, verificará se algo está escrito na EEPROM, e se estiver, ele manda um novo status para o ELIPSE
  if(solicitacao != 0)
  {
    Serial.println("Algo na Memória: ");
    Serial.println(solicitacao);
    MQTT.publish(STATUS_PAGER.c_str(), "6", MQTTpubQos); //Manda Status 6 e espera Elipse DITAR o estado que eu deveria estar
  }
  else{
    Serial.println("Nada na Memória");
  }
  EEPROMdata.end();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          VOIDS PRINCIPAIS                                               //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  //para debugar
  Serial.begin(115200);

  //Avisa início de Sistema
  Serial.println("Sistema Inicializado");

  //Configuraçãoes MQTT
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); //Define porta e endereço do servidor broker mqtt 
  MQTT.setCallback(callback);

  //configurações display
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  //Configurações de Hardware
  pinMode(btbaixo, INPUT_PULLUP);
  pinMode(btcima, INPUT_PULLUP);
  pinMode(btenter, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  //Configurações WIFI
  conectaWiFi();
  connectMQTT();
  WiFi.mode(WIFI_AP_STA); 
  Serial.println(WiFi.macAddress());
  
  //Configurações de EEPROM
  EEPROMdata.begin(EEPROM_SOLICITACOES);//Inicia a meória, criando um bloco de dados dentro dela
  //EEPROMdata.putFloat(EEPROM_ENDERECO_SOLICITACOES, 0);   //usado para limpar a memória nos testes
  verificaMemoria(EEPROMdata.getFloat(EEPROM_ENDERECO_SOLICITACOES, 0));
  //Configurações de tags
  sts_sistema = ""; //futuramente receberá o conteúdo de verificação de conexões do sistema
}

void loop() {
  //MQTT e WiFi
  processaDesconexoes();

  //Controle de Botões Para Menu
  controle_navegacao(btcima, btenter, btbaixo);

  //Serial.print("|||||||||");
  Serial.println(navegacao_status);
  //Serial.println(local);
  //Serial.println(produto);
  //Serial.println(status_chamada);
  // Serial.println(navegadoOP);
  //Serial.print(digitalRead(btcima));
  //Serial.print(digitalRead(btbaixo));
  //Serial.println(digitalRead(btenter));
  Serial.println(selecionadoFinalParada);
  //Controle de Aparições no Display
  controle_display(sts_sistema, MQTT.state());
}