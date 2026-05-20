## 编译 Agora SDK 示例（适用于 Linux）

```sh
$ cd example/
$ ./build.sh
```

编译成功后，会在 `out` 目录下生成一个名为 `sample_recorder` 的可执行程序 和 `recorder_local`的可执行程序。
其中，`recorder_local`可执行程序是仿照老Linux录制sdk的recorder_local可执行程序.

## 运行 Agora SDK 示例
- 运行sample_recorder
你需要一个编写一个json文件，设置一系列录制参数。`recorder.json`文件是一个示例。
```
$ LD_LIBRARY_PATH=../agora_sdk/ ./out/sample_recorder recorder.json
```
### json文件参数说明
```
{
    "appId": "*****",               // App ID
    "token": "*****",               // 认证 Token
    "channelName": "recorderTest",   // 频道名
    "useStringUid": false,           // 是否使用string uid
    "useCloudProxy": false,         // 是否使用云代理
    "userId": "0",                  // 用户Id，默认填"0"
    "subAllAudio": true,         // 是否订阅所有音频，false 时在 subAudioUserList 填入订阅的 UID
    "subAudioUserList": ["123"],  
    "subAllVideo": true,         // 是否订阅所有视频，false 时在 subVideoUserList 填入订阅的 UID
    "subVideoUserList": ["123"],
    "subStreamType":"high",      // 订阅大小流， high（大流），low（小流）
    "isMix":true,             // 是否合流录制
    "backgroundColor": 0,     // 合流录制背景颜色，0xRRGGBB 格式（红:0xFF0000，绿:0x00FF00，蓝:0x0000FF）
    "backgroundImage": "1.jpg",  // 合流录制设置背景图片,支持png和jpg，当同时设置了背景颜色和背景图片，优先使用背景图片
    "layoutMode":"bestfit",      // 合流录制布局（default: 默认布局, bestfit: 自适应布局, vertical: 垂直布局）
    "maxResolutionUid": "123",   // vertical 布局中，最大分辨率的 UID
    "recorderStreamType":"both",   // 录制类型（audio_only: 仅音频, video_only: 仅视频, both: 音视频）
    "recorderPath":"recorderTest.mp4",  // 合流录制时的文件名。单流录制时的目录
    "maxDuration": 120,  // 录制时长（秒）
    "recoverFile": false,   // 是否在录制时同时写h264和aac文件，程序crash后可以恢复出mp4
    "audioVolumeIndicationIntervalMs": 500,   //音量回调时间间隔
    "audio":{
        "sampleRate": 16000,  // 音频采样率
        "numOfChannels": 1    // 音频通道数
    },
    "video":{
        "width":1920,        // 视频宽
        "height":1080,       // 视频高
        "fps":15             // 视频帧率
    },
    "waterMark":[            //合流录制水印设置（可选）
       {
            "type": "litera",                                                  // 字幕水印
            "litera":"你好,recorder test,ABC1234",                              // 字幕
            "fontFilePath":"./Noto_Sans_SC/static/NotoSansSC-Regular.ttf",      // 字体路径
            "fontSize": 15,                                                     // 字体大小
            "x": 0,                                                             // 水印起点x坐标
            "y": 0,                                                             // 水印起点y坐标
            "width": 400,                                                       // 水印宽
            "height":200,                                                       // 水印高
            "zorder":0                                                          // 水印间层级，数字越大显示越上层
        },
        {           
            "type": "time",                                                     // 时间戳水印
            "fontFilePath":"./Noto_Sans_SC/static/NotoSansSC-Regular.ttf",
            "fontSize": 15,
            "x": 0,
            "y": 800,
            "width": 400,
            "height":200,
            "zorder":0
        },
        {
            "type": "picture",          // 图片水印
            "imgUrl":"xxx.png",
            "x": 0,
            "y": 0,
            "width": 100,
            "height":100,
            "zorder":0
        }
    ],
    
    "encryption":{              // 媒体流加密
        "mode": "AES_128_ECB",  // 加密类型 AES_128_XTS, AES_128_ECB, AES_256_XTS, SM4_128_ECB, AES_128_GCM, AES_256_GCM, AES_128_GCM2, AES_256_GCM2
        "key": "xxx",           
        "salt": "xxx"           // 32位字符串
    },
    "rotation":[                    // 画面旋转
        {
            "uid": "123",
            "degree": 90,           // 旋转的角度，0，90，180，270
        }
    ],
    "localAp": {
        "mode": 0,              // ConnectivityFirst = 0, LocalOnly = 1,
        "verifyDomainName": "123"      // verifyDomainName
        "domainList" : [
            "xxx",
            "xxx",
        ],
        "ipList": [
            "xxxx",
        ]
    },
    "frameCapture": {                // 获取视频帧
        "videoFrameType": "jpg_file",      // "encoded_frame",h264/5帧， "yuv_frame": yuv帧, "jpg_frame": jpg帧，"jpg_file": jpg文件
        "jpgCaptureInterval": 1,
        "jpgFileStorePath": "./".         // jpg文件存储位置
    }
}
```

- 运行recorder_local
```
$ LD_LIBRARY_PATH=../agora_sdk ./out/recorder_local --appId xxx --channelKey xxx --channel xxx --isMixingEnabled 0 --recordFileRootDir . --autoSubscribe 1
更多命令行参数可参考usage 
```