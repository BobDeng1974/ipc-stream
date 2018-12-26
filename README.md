# ipc-stream
旨在为嵌入式摄像头提供可控的rtmp 推流功能，功能特性如下
- [x] 支持RTMP推流
- [x] 支持H.264 视频帧
- [x] 支持AAC/G.711A/G711U 音频帧
- [x] 支持mqtt信令
- [x] 低内存占用
- [x] 支持 PCM 16bit 音频数据编码
- [x] sdk内部解码nalu和adts

## 编译

#### X86
- mkdir build
- cd build
- cmake ..
- make

#### ARM
- mkdir build-arm
- cd build-arm
- cmake .. -DARM=yes
- make

## sample概述
- 使用mqtt主题为"pushLive"
- 收到信令"pushLiveStart"开始推流，此时需要发送信令"pushSucceed"给客户端
- 收到信令"pushLiveStop"停止推流

