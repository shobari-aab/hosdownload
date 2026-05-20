## Compile Agora SDK Example (for Linux)

```sh
$ cd example/
$ ./build.sh
```

After successful compilation, an executable file named sample_recorder will be generated in the out directory.

## Run Agora SDK Example
You need to create a JSON file to set a series of recording parameters. The recorder.json file is an example.

```sh
$ LD_LIBRARY_PATH=../agora_sdk/ ./out/sample_recorder recorder.json
```

## JSON File Parameter Description
```
{
    "appId": "*****",               // App ID
    "token": "*****",               // Authentication Token
    "channelName": "recorderTest",  // Channel name
    "useStringUid": false,          // Whether to use string UID
    "useCloudProxy": false,         // Whether to use cloud proxy
    "userId": "0",                  // User ID, default is "0"
    "subAllAudio": true,            // Whether to subscribe to all audio, if false, fill in the subscribed UIDs in subAudioUserList
    "subAudioUserList": ["123"],  
    "subAllVideo": true,            // Whether to subscribe to all video, if false, fill in the subscribed UIDs in subVideoUserList
    "subVideoUserList": ["123"],
    "subStreamType": "high",        // Subscribe to high or low stream, high (large stream), low (small stream)
    "isMix": true,                  // Whether to mix recording
    "backgroundColor": 0,           // Background color for mixed recording, in 0xRRGGBB format (Red: 0xFF0000, Green: 0x00FF00, Blue: 0x0000FF)
    "backgroundImage": "1.jpg",     // Background image for mixed recording, supports png and jpg, if both background color and image are set, image takes precedence
    "layoutMode": "bestfit",        // Layout mode for mixed recording (default: default layout, bestfit: adaptive layout, vertical: vertical layout)
    "maxResolutionUid": "123",      // UID with the highest resolution in vertical layout
    "recorderStreamType": "both",   // Recording type (audio_only: audio only, video_only: video only, both: audio and video)
    "recorderPath": "recorderTest.mp4",  // File name for mixed recording. Directory for single stream recording
    "maxDuration": 120,             // Recording duration (seconds)
    "recoverFile": false,           // Whether to write h264 and aac files simultaneously during recording, can recover mp4 if program crashes
    "audioVolumeIndicationIntervalMs": 500,   // Volume callback interval
    "audio": {
        "sampleRate": 16000,        // Audio sample rate
        "numOfChannels": 1          // Number of audio channels
    },
    "video": {
        "width": 1920,              // Video width
        "height": 1080,             // Video height
        "fps": 15                   // Video frame rate
    },
    "waterMark": [                  // Watermark settings for mixed recording (optional)
        {
            "type": "litera",       // Subtitle watermark
            "litera": "Hello, recorder test, ABC1234",  // Subtitle text
            "fontFilePath": "./Noto_Sans_SC/static/NotoSansSC-Regular.ttf",  // Font file path
            "fontSize": 15,         // Font size
            "x": 0,                 // X coordinate of watermark start point
            "y": 0,                 // Y coordinate of watermark start point
            "width": 400,           // Watermark width
            "height": 200,          // Watermark height
            "zorder": 0             // Watermark layer order, higher value means higher layer
        },
        {           
            "type": "time",         // Timestamp watermark
            "fontFilePath": "./Noto_Sans_SC/static/NotoSansSC-Regular.ttf",
            "fontSize": 15,
            "x": 0,
            "y": 800,
            "width": 400,
            "height": 200,
            "zorder": 0
        },
        {
            "type": "picture",      // Picture watermark
            "imgUrl": "xxx.png",
            "x": 0,
            "y": 0,
            "width": 100,
            "height": 100,
            "zorder": 0
        }
    ],
    "encryption": {                 // Media stream encryption
        "mode": "AES_128_ECB",      // Encryption type: AES_128_XTS, AES_128_ECB, AES_256_XTS, SM4_128_ECB, AES_128_GCM, AES_256_GCM, AES_128_GCM2, AES_256_GCM2
        "key": "xxx",               
        "salt": "xxx"               // 32-character string
    },
    "rotation": [                   // Video rotation
        {
            "uid": "123",
            "degree": 90            // Rotation angle: 0, 90, 180, 270
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
    "frameCapture": {                // capture video frame
        "videoFrameType": "jpg_file",      // "encoded_frame" for h264/5， "yuv_frame" for yuv, "jpg_frame" for jpg，"jpg_file": for jpg files
        "jpgCaptureInterval": 1,
        "jpgFileStorePath": "./".         // jpg file saved path
    }
}
```