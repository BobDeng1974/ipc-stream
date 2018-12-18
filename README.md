# ipc-stream
Controllable rtmp stream
旨在为嵌入式摄像头提供 可控的rtmp 推流功能，功能特性如下
- [x] 支持RTMP推流
- [x] 支持H.264 视频帧
- [x] 支持AAC/G.711A/G711U 音频帧
- [x] 支持mqtt信令

## 编译
git submodule update --init --recursive

- mkdir build
- cd build
- cmake ..
- make

## sdk核心API
#### 推流
- RtmpNewContext,新建推流上下文
- RtmpSendVideo,发送视频数据
- RtmpSendAudio,发送音频数据
- RtmpConnect,连接RTMP服务器

#### 信令
- MqttNewContex,新建MQTT上下文
- MqttDestroyContex,销毁MQTT上下文
- MqttYield，轮询消息
- MqttSend,发送MQTT消息
