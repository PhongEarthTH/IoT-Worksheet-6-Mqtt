import paho.mqtt.client as mqtt
import json
import requests

host = "192.168.124.66"
port = 1883
def on_connect(self, client, userdata, rc):
    print("MQTT Connected.")
    self.subscribe("DHT")
def on_message(client, userdata,msg):
    print(msg.payload.decode("utf-8", "strict"))
    data = json.loads(msg.payload.decode("utf-8", "strict"))
    print(data)
    serverurl = "http://192.168.124.66:3000/data"
    res = requests.post(serverurl, json=data)
    print(res.status_code)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(host)
client.loop_forever()