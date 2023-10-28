#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Servo.h>

#define sensor_Pin   0    
#define DHTPIN D2
#define DHTTYPE DHT11
long s=0;  //时间
unsigned long lastMs = 0; //时间

float A0_data = 0;//esp8266 A0口的模拟数据
bool led_state = true;//led灯的状态
bool pow_state = false;

const int IN1 = D6; //定义电机引脚为D6

float h = 0.0;//湿度
float t = 0.0;//温度

DHT dht(DHTPIN,DHTTYPE);


// 连接WIFI和密码 
#define WIFI_SSID         "lmxddad"
#define WIFI_PASSWD       "lzwlzw1108"

//设备的三元组信息 在阿里云设备--查看
#define PRODUCT_KEY       "gvo4EX9zxaa"
#define DEVICE_NAME       "humTemp"
#define DEVICE_SECRET     "de33b9483742a983cc66e5a6bf22cd0f"
#define REGION_ID         "cn-shanghai"

//不需要改 
#define MQTT_SERVER       PRODUCT_KEY ".iot-as-mqtt." REGION_ID ".aliyuncs.com"
#define MQTT_PORT         1883
#define MQTT_USRNAME      DEVICE_NAME "&" PRODUCT_KEY

//密码和密钥
#define CLIENT_ID     "gvo4EX9zxaa.humTemp|securemode=2,signmethod=hmacsha256,timestamp=1697691849403|"
#define MQTT_PASSWD   "b5eee7fe6091e1aaba0c54fb881e35ae7cc68bb55d6d7dd394495bd447fa10a9"

//ESP8266发送数据格式(cjson格式)
#define ALINK_BODY_FORMAT         "{\"id\":\"ESP8266\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":12%s}"
//ESP8266发送主题 "/sys/a1kCPZ9IyXn/test001/thing/event/property/post";
#define ALINK_TOPIC_PROP_POST     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
//ESP8266订阅接收主题 "/sys/a1kCPZ9IyXn/test001/thing/service/property/set";
#define ALINK_TOPIC_PROP_Set     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/set"

//创建esp8266 wifi的客户端实例，做客户端连接某路由器
WiFiClient espClient;
//创建esp8266 mqtt的客户端实例，为物联网终端设备，连接阿里云IOT,订阅发布消息，实现物联。
PubSubClient  mqttClient(espClient);

  

//连接wifi
void wifiInit()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("WiFi not Connect");
    }

    mqttClient.setBufferSize(1024); //设置buffer大小
    mqttClient.setKeepAlive(70);    //设置存活时间
    mqttClient.setSocketTimeout(90);    //设置socket超时时间
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);   //连接MQTT服务器 
    mqttClient.setCallback(recvMsg);  // 设置回调函数
}

//mqtt连接
void mqttCheckConnect()
{
    while (!mqttClient.connected())
    {
        mqttClient.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD);
    }

    if (mqttClient.subscribe(ALINK_TOPIC_PROP_Set)) {
      Serial.println("MQTT主题订阅成功");
    } else {
      Serial.println("MQTT主题订阅失败");
    }
}

void mqttIntervalPost()
{
    A0_data = analogRead(sensor_Pin);   //模拟数据读取（A0脚模拟输入的数据）
    
    char param[32];
    char jsonBuf[128];
    read_data();
    sprintf(param, "{\"Temp\":%f}",t);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);
    sprintf(param, "{\"Hum\":%f}",h);
    //这里\"**"\要选择对应的标识符，否则数据发不过去。
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);
    boolean e = mqttClient.publish(ALINK_TOPIC_PROP_POST, jsonBuf);

    sprintf(param, "{\"LEDSwitch\":%d}",led_state);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);

     sprintf(param, "{\"PowerSwitch\":%d}",pow_state);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);

   
     boolean f = mqttClient.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
  
}

//接收订阅主题所发送来的数据，并打印到串口
void recvMsg(char *topic, uint8_t *payload, size_t length) {
  Serial.println("topic: " + String(topic));
  Serial.println("payload: " + String((char *)payload).substring(0, length));
  Serial.println("length: " + String(length));

  /* 解析JSON */
  StaticJsonDocument<200> jsonBuffer2; //声明一个JsonDocument对象，长度200
  // 反序列化JSON
  DeserializationError error = deserializeJson(jsonBuffer2, String((char *)payload).substring(0, length));
  if (error) 
  {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
  }
  //提取出参数的数据
  const char* str = jsonBuffer2["id"];           // 读取字符串
  Serial.println(str);
  //读取温度参数
  double t_val = jsonBuffer2["params"]["Temp"];       // 读取嵌套对象  
  Serial.println(t_val);
  double h_val = jsonBuffer2["params"]["Hum"];       // 读取嵌套对象  
  Serial.println(h_val);
 
  //读取led灯命令
  int switch_led_val = jsonBuffer2["params"]["LEDSwitch"];  // 读取嵌套对象  
  Serial.println(switch_led_val);
  if(switch_led_val == 0){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else if(switch_led_val == 1)
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
  else{

  }




 //读取电机命令
  int km_val = jsonBuffer2["params"]["PowerSwitch"];  // 读取嵌套对象  
  Serial.println(km_val);
  if(km_val == 0){
    digitalWrite(IN1,  LOW);
  }
  else if(km_val == 1)
  {
    digitalWrite(IN1,  HIGH);
  }




}

//采样任意传感器数据
void read_data()
{

   h=dht.readHumidity();
   t=dht.readTemperature();
   //读取数据程序.....
}

void setup() 
{
    Serial.begin(9600);
    wifiInit();
    dht.begin();
    //led灯
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(IN1,OUTPUT);
   

}

void loop()
{
  s=millis();
  delay(500);
  read_data();
  Serial.println(millis()-s);

  if (millis() - lastMs >= 5000) //定时间隔
  {
      lastMs = millis();
      mqttCheckConnect(); 
      mqttIntervalPost();
  }
  mqttClient.loop();
  delay(2000);
}
