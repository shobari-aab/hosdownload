#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

 enum LAYOUT_MODE_TYPE {
  DEFAULT_LAYOUT = 0,
  BESTFIT_LAYOUT = 1,
  VERTICALPRESENTATION_LAYOUT = 2,
};

enum SUB_VIDEO_STREAM_TYPE{
  STREAM_HIGH = 0,
  STREAM_LOW = 1,
};

enum RECORDER_STERAM_TYPE{
  RECORDER_AUDIO_ONLY = 0x01,
  RECORDER_VIDEO_ONLY = 0x02,
  RECORDER_BOTH = RECORDER_AUDIO_ONLY | RECORDER_VIDEO_ONLY,
};

enum ENCRYPTION_MODE {
  AES_128_XTS = 1,
  AES_128_ECB = 2,
  AES_256_XTS = 3,
  SM4_128_ECB = 4,
  AES_128_GCM = 5,
  AES_256_GCM = 6,
  AES_128_GCM2 = 7,
  AES_256_GCM2 = 8,
  MODE_END,
};
 
enum LOCAL_PROXY_MODE {
  ConnectivityFirst = 0,
  LocalOnly = 1,
};

struct waterMarkPos{
  int x;
  int y;
  int width;
  int height;
  int zorder;
};

struct literaWaterMark{
  std::string litera;
  std::string fontFilePath;
  int fontSize;
};
struct timeWaterMark{
  std::string fontFilePath;
  int fontSize;
};

struct pictureWaterMark{
  std::string image_url;
};

enum WaterMarkType {
  None = 0,
  Literal,
  Time,
  Picture
};
struct WaterMark {
  WaterMarkType type;
  literaWaterMark literalMark;
  timeWaterMark timeMark;
  pictureWaterMark pictureMark;
  waterMarkPos pos;

  WaterMark() : type(WaterMarkType::None) {}
};

struct FrameCaptureConfig {
  bool enable = false;
  int videoFrameType = 0;
  int jpgCaptureInterval = 5;
  std::string jpgFileStorePath;
};

struct recorder_config
{
  std::string appId;
  std::string token;
  std::string ChannelName;
  bool UseStringUid = false;
  bool UseCloudProxy = false;
  std::string UserId;
  bool SubAllAudio = true;
  std::vector<std::string> SubAudioUserList;
  bool SubAllVideo = true;
  std::vector<std::string> SubVideoUserList;
  int subStreamType = STREAM_HIGH;   // 0: high 1: low
  bool isMix = false;
  int layoutMode = DEFAULT_LAYOUT;
  uint32_t backgroundColor = 0;
  std::string backgroundImage;
  std::string maxResolutionUid;
  std::string recorderPath;
  std::string callID;
  int recorderStreamType = RECORDER_BOTH;
  int maxDuration = 120;
  bool recoverFile = false;
  int audioVolumeIndicationIntervalMs = 0;
  struct {
      int sampleRate = 16000;
      int numOfChannels = 1;
  } audio;
  struct {
      int fps = 15;
      int height = 1080;
      int width = 1920;
  } video;
    
  std::vector<WaterMark> waterMarks;

  bool enableEncryption = false;
  struct {
      ENCRYPTION_MODE mode;
      std::string key;
      std::string salt;
  } encryption;

  std::unordered_map<std::string, int> rotationMap;

  bool UseLocalAp = false;
  struct {
    LOCAL_PROXY_MODE mode;
    std::string verifyDomainName;
    std::vector<std::string> ipList;
    std::vector<std::string> domainList;
  } localAp;

  FrameCaptureConfig frameCaptureConfig;
};

void printRecorderConfig(const recorder_config &config);
recorder_config getRecorderConfigbyJson(const char *filename);
