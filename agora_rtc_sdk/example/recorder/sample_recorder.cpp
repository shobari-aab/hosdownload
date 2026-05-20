#include "AgoraBase.h"
#include "parser/parser.h"
#include "IAgoraService.h"
#include "IAgoraMediaComponentFactory.h"
#include "IAgoraMediaRtcRecorder.h"
#include "recorderEventHandler.h"
#include <memory>
#include <csignal>
#include "log.h"
#include <unistd.h>

static bool exitFlag = false;
static void SignalHandler(int sigNo) { exitFlag = true; }

int main(int argc, char* argv[]) 
{
  if ((argc < 2)){
    AG_LOG(ERROR, "usage: ./%s json_file", argv[0]);
    return -1;
  }

  recorder_config config = getRecorderConfigbyJson(argv[1]);
  printRecorderConfig(config);

  if(config.appId.empty() || config.token.empty() || config.ChannelName.empty()){
    AG_LOG(ERROR, "appid or token or channelName empty!!!");
    return -1;
  }

  std::signal(SIGQUIT, SignalHandler);
  std::signal(SIGABRT, SignalHandler);
  std::signal(SIGINT, SignalHandler);

  auto service = createAgoraService();
  agora::base::AgoraServiceConfiguration service_config;
  service_config.enableAudioDevice = false;
  service_config.enableAudioProcessor = true;
  service_config.enableVideo = true;
  service_config.appId = config.appId.c_str();
  service_config.useStringUid = config.UseStringUid;
  service_config.logConfig.filePath = "./io.agora.rtc_sdk/agorasdk.log";
  service_config.logConfig.fileSizeInKB = 1024 * 100; // 100MB
  service->initialize(service_config);

  if (config.UseCloudProxy) {
    auto agoraParameter = service->getAgoraParameter();
    agoraParameter->setBool("rtc.enable_proxy", true);
    AG_LOG(INFO, "set the Cloud_Proxy Open!");
  }

  if(config.recoverFile){
    auto agoraParameter = service->getAgoraParameter();
    agoraParameter->setBool("che.media_recorder_recover_files", true);
  }

  agora::rtc::IMediaComponentFactory* factory = createAgoraMediaComponentFactory();
  agora::agora_refptr<agora::rtc::IAgoraMediaRtcRecorder> recorder =  factory->createMediaRtcRecorder();
  recorder->initialize(service, config.isMix);
  
  std::unique_ptr<RecorderEventHandler> eventHandler{new RecorderEventHandler(recorder, config)};
  recorder->registerRecorderEventHandle(eventHandler.get());
  if(config.enableEncryption){
    agora::rtc::EncryptionConfig encryptConfig;
    encryptConfig.encryptionMode = static_cast<agora::rtc::ENCRYPTION_MODE>(config.encryption.mode);
    encryptConfig.encryptionKey = config.encryption.key.c_str();
    if(!config.encryption.salt.empty()){
      memcpy(encryptConfig.encryptionKdfSalt, config.encryption.salt.data(), 32);
    }
    recorder->enableEncryption(true, encryptConfig);
  }

  std::unique_ptr<RecorderVideoFrameObserver> videoFrameObserver{new RecorderVideoFrameObserver()};
  agora::rtc::RecorderVideoFrameCaptureConfig videoFrameCaptureConfig;
  if(config.frameCaptureConfig.enable){
    videoFrameCaptureConfig.videoFrameType = static_cast<agora::rtc::VideoFrameCaptureType>(config.frameCaptureConfig.videoFrameType);
    videoFrameCaptureConfig.jpgCaptureIntervalInSec = config.frameCaptureConfig.jpgCaptureInterval;
    videoFrameCaptureConfig.jpgFileStorePath = config.frameCaptureConfig.jpgFileStorePath.c_str();
    videoFrameCaptureConfig.observer = videoFrameObserver.get();
    recorder->enableRecorderVideoFrameCapture(true, videoFrameCaptureConfig);
  }

  if(config.SubAllAudio){
    recorder->subscribeAllAudio();
  } else {
    for(const auto& it:config.SubAudioUserList){
      recorder->subscribeAudio(it.c_str());
    }
  }
  if(config.UseLocalAp){
    agora::rtc::LocalAccessPointConfiguration localApConfig;
    localApConfig.mode = static_cast<agora::rtc::LOCAL_PROXY_MODE>(config.localAp.mode);
    localApConfig.verifyDomainName = config.localAp.verifyDomainName.c_str();
    localApConfig.domainListSize = config.localAp.domainList.size();
    std::vector<const char*> cdomainList;
    for (const auto& domain : config.localAp.domainList) {
        cdomainList.push_back(domain.c_str());
    }
    localApConfig.domainList = cdomainList.data();
    localApConfig.ipListSize = config.localAp.ipList.size();
    std::vector<const char*> cipList;
    for (const auto& ip : config.localAp.ipList) {
        cipList.push_back(ip.c_str());
    }
    localApConfig.ipList = cipList.data();
    recorder->setGlobalLocalAccessPoint(localApConfig);
  }

  agora::rtc::VideoSubscriptionOptions options;
  options.encodedFrameOnly = false;
  options.type = static_cast<agora::rtc::VIDEO_STREAM_TYPE>(config.subStreamType);
  if(config.SubAllVideo){
    recorder->subscribeAllVideo(options);
  } else {
    for(const auto& it : config.SubVideoUserList){
      recorder->subscribeVideo(it.c_str(), options);
    }
  }

  recorder->setAudioVolumeIndicationParameters(config.audioVolumeIndicationIntervalMs);

  recorder->joinChannel(config.token.c_str(), config.ChannelName.c_str(), config.UserId.c_str());
  
  if(config.isMix){
    agora::media::MediaRecorderConfiguration recorder_config;
    recorder_config.width = config.video.width;
    recorder_config.height = config.video.height;
    recorder_config.fps = config.video.fps;
    recorder_config.storagePath = config.recorderPath.c_str();
    recorder_config.sample_rate = config.audio.sampleRate;
    recorder_config.channel_num = config.audio.numOfChannels;
    recorder_config.streamType = static_cast<agora::media::MediaRecorderStreamType>(config.recorderStreamType) ;
    recorder_config.maxDurationMs = config.maxDuration * 1000;
    recorder->setRecorderConfig(recorder_config);
      
    agora::rtc::WatermarkConfig watermarks[config.waterMarks.size()];
    for(int i = 0; i < config.waterMarks.size(); i++){
      watermarks[i].index = i+1;
      if(config.waterMarks[i].type == Literal){
        watermarks[i].type = agora::rtc::LITERA;
        watermarks[i].literaSource.wmLitera = config.waterMarks[i].literalMark.litera.c_str();
        watermarks[i].literaSource.fontFilePath = config.waterMarks[i].literalMark.fontFilePath.c_str();
        watermarks[i].literaSource.fontSize = config.waterMarks[i].literalMark.fontSize;
      } else if(config.waterMarks[i].type == Time) {
        watermarks[i].type = agora::rtc::TIMESTAMPS;
        watermarks[i].timestampSource.fontFilePath = config.waterMarks[i].timeMark.fontFilePath.c_str();
        watermarks[i].timestampSource.fontSize = config.waterMarks[i].timeMark.fontSize;
      } else if(config.waterMarks[i].type == Picture){
        watermarks[i].type = agora::rtc::PICTURE;
        watermarks[i].imageUrl = config.waterMarks[i].pictureMark.image_url.c_str();
      }

      watermarks[i].options.mode = agora::rtc::FIT_MODE_COVER_POSITION;
      watermarks[i].options.zOrder = config.waterMarks[i].pos.zorder;
      watermarks[i].options.positionInPortraitMode.x = config.waterMarks[i].pos.x;
      watermarks[i].options.positionInPortraitMode.y = config.waterMarks[i].pos.y;
      watermarks[i].options.positionInPortraitMode.width = config.waterMarks[i].pos.width;
      watermarks[i].options.positionInPortraitMode.height = config.waterMarks[i].pos.height;
      watermarks[i].options.positionInLandscapeMode.x = config.waterMarks[i].pos.x;
      watermarks[i].options.positionInLandscapeMode.y = config.waterMarks[i].pos.y;
      watermarks[i].options.positionInLandscapeMode.width = config.waterMarks[i].pos.width;
      watermarks[i].options.positionInLandscapeMode.height = config.waterMarks[i].pos.height;
    }
    recorder->enableAndUpdateVideoWatermarks(watermarks, config.waterMarks.size());
    
    recorder->startRecording();

    if(config.backgroundColor != 0 || !config.backgroundImage.empty())
    {
      agora::rtc::VideoMixingLayout layout;
      layout.canvasWidth = config.video.width;
      layout.canvasHeight = config.video.height;
      layout.canvasFps = config.video.fps;
      layout.backgroundColor = config.backgroundColor;
      if(config.backgroundImage.empty()){
        layout.backgroundImage = NULL;
      }else {
        layout.backgroundImage = config.backgroundImage.c_str();
      }
      layout.userLayoutConfigNum = 0;
      recorder->setVideoMixingLayout(layout);
    }
  }

  while (!exitFlag) {
    usleep(10000);
  }

  if(config.SubAllAudio){
    recorder->unsubscribeAllAudio();
  }
  if(config.SubAllVideo){
    recorder->unsubscribeAllVideo();
  }
  if(config.isMix){
    recorder->stopRecording();
  } else {
    std::set<std::string> uids = eventHandler->getUids();
    for(auto& uid : uids){
      recorder->stopSingleRecordingByUid(uid.c_str());
    }
  }
  if(config.frameCaptureConfig.enable){
    recorder->enableRecorderVideoFrameCapture(false, videoFrameCaptureConfig);
  }
  recorder->unregisterRecorderEventHandle(eventHandler.get());
  eventHandler = nullptr;
  videoFrameObserver = nullptr;
  recorder->leaveChannel();
  recorder = nullptr;
  service->release();
  return 0;
}