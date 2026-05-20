#include "recorderEventHandler.h"
#include "IAgoraMediaRtcRecorder.h"
#include <string>
#include <vector>
#include <string>
#include "log.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>


std::string getCurrentTimeAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&now_time_t);

    std::ostringstream oss;
     char buffer[100];
    if (std::strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H:%M:%S", &local_time)) {
        oss << buffer;
    } else {
        oss.setstate(std::ios_base::failbit);
    }
    // oss << std::put_time(&local_time, "%Y_%m_%d_%H:%M:%S");
    return oss.str();
}

// layout
void RecorderEventHandler::setVideoMixLayout()
{
  if(uid_resolutions_.size()> 0 && uid_resolutions_.size() <= 17){
    agora::rtc::VideoMixingLayout layout;
    layout.canvasWidth = config_.video.width;
    layout.canvasHeight = config_.video.height;
    layout.canvasFps = config_.video.fps;
    layout.userLayoutConfigNum = uid_resolutions_.size();
    layout.backgroundColor = config_.backgroundColor;
    if(config_.backgroundImage.empty()){
      layout.backgroundImage = NULL;
    }else {
      layout.backgroundImage = config_.backgroundImage.c_str();
    }
    agora::rtc::UserMixerLayout use_layout[uid_resolutions_.size()];

    if(config_.layoutMode == BESTFIT_LAYOUT){
        adjustBestFitVideoLayout(use_layout);
    } else if(config_.layoutMode == DEFAULT_LAYOUT){
        adjustDefaultVideoLayout(use_layout);
    } else if(config_.layoutMode == VERTICALPRESENTATION_LAYOUT){
        adjustVerticalPresentationLayout(use_layout, config_.maxResolutionUid);
    }

    for(int i = 0; i < uid_resolutions_.size(); i++){
      if(config_.rotationMap.find(use_layout[i].userId) != config_.rotationMap.end()){
        use_layout[i].config.rotation = config_.rotationMap[use_layout[i].userId];
      }
    }

    layout.userLayoutConfigs = use_layout;
    recorder_->setVideoMixingLayout(layout);
  }
}

void scaleWidthHeight(int videoWidth, int videoHeight, int canvasWidth, int canvasHeight, int &scaledWidth, int &scaledHeight) 
{
  float videoAspectRatio = static_cast<float>(videoWidth) / videoHeight;
  float canvasAspectRatio = static_cast<float>(canvasWidth) / canvasHeight;

  if (videoAspectRatio > canvasAspectRatio) {
    scaledWidth = canvasWidth;
    scaledHeight = static_cast<int>(canvasWidth / videoAspectRatio);
  } else {
    scaledHeight = canvasHeight;
    scaledWidth = static_cast<int>(canvasHeight * videoAspectRatio);
  }
}

void RecorderEventHandler::adjustBestFitLayout_Square(agora::rtc::UserMixerLayout* regionList, int nSquare) 
{
  float viewWidth = static_cast<float>(1.f * 1.0/nSquare);
  float viewHEdge = static_cast<float>(1.f * 1.0/nSquare);
  int i = 0;
  for(const auto& uid : uid_resolutions_){
    float xIndex =static_cast<float>(i%nSquare);
    float yIndex = static_cast<float>(i/nSquare);
    regionList[i].userId = uid.first.c_str();

    int x = 1.f * 1.0/nSquare * xIndex * config_.video.width;
    int y = 1.f * 1.0/nSquare * yIndex * config_.video.height;
    int videoWidth = uid.second.width;
    int videoHeight = uid.second.height;
    int canvasWidth = viewWidth * config_.video.width;
    int canvasHeight = viewHEdge * config_.video.height;
    int scaledWidth, scaledHeight;
    scaleWidthHeight(videoWidth, videoHeight, canvasWidth, canvasHeight, scaledWidth, scaledHeight);
    regionList[i].config.x = x + (canvasWidth - scaledWidth) / 2;
    regionList[i].config.y = y + (canvasHeight - scaledHeight) / 2;
    regionList[i].config.width = scaledWidth;
    regionList[i].config.height = scaledHeight;

    i++;
  }
}

void RecorderEventHandler::adjustBestFitLayout_2(agora::rtc::UserMixerLayout* regionList) 
{
  int i = 0;
  for(const auto& uid : uid_resolutions_){
    regionList[i].userId = uid.first.c_str();
    
    int x = (((i+1)%2)?0:0.5) * config_.video.width;
    int y = 0;
    int videoWidth = uid.second.width;
    int videoHeight = uid.second.height;
    int canvasWidth = 0.5f * config_.video.width;
    int canvasHeight = 1.f * config_.video.height;
    int scaledWidth, scaledHeight;
    scaleWidthHeight(videoWidth, videoHeight, canvasWidth, canvasHeight, scaledWidth, scaledHeight);

    regionList[i].config.x = x + (canvasWidth - scaledWidth) / 2;
    regionList[i].config.y =  y + (canvasHeight - scaledHeight) / 2;
    regionList[i].config.width = scaledWidth;
    regionList[i].config.height = scaledHeight;
    i++;
  }
}

void RecorderEventHandler::adjustBestFitLayout_17(agora::rtc::UserMixerLayout* regionList) 
{
  int n = 5;
  float viewWidth = static_cast<float>(1.f * 1.0/n);
  float viewHEdge = static_cast<float>(1.f * 1.0/n);
  
  int i = 0;
  for(const auto& uid : uid_resolutions_){
    float xIndex = static_cast<float>(i%(n-1));
    float yIndex = static_cast<float>(i/(n-1));
    regionList[i].userId = uid.first.c_str();

    int x = ((i == 16) ? ((1-viewWidth)*(1.f/2) * 1.f) * config_.video.width : (0.5f * viewWidth + viewWidth * xIndex)) * config_.video.width;
    int y = (1.f * 1.0/n) * yIndex * config_.video.height;
    int videoWidth = uid.second.width;
    int videoHeight = uid.second.height;
    int canvasWidth = viewWidth * config_.video.width;
    int canvasHeight = viewHEdge * config_.video.height;
    int scaledWidth, scaledHeight;
    scaleWidthHeight(videoWidth, videoHeight, canvasWidth, canvasHeight, scaledWidth, scaledHeight);
    
    regionList[i].config.x = x + (canvasWidth - scaledWidth) / 2;
    regionList[i].config.y = y + (canvasHeight - scaledHeight) / 2;
    regionList[i].config.width = scaledWidth;
    regionList[i].config.height = scaledHeight;
    
    i++;
  }
}

void RecorderEventHandler::adjustBestFitVideoLayout(agora::rtc::UserMixerLayout* regionList) 
{
  if(uid_resolutions_.size() == 1) {
      adjustBestFitLayout_Square(regionList,1);
  }else if(uid_resolutions_.size() == 2) {
      adjustBestFitLayout_2(regionList);
  }else if( 2 < uid_resolutions_.size() && uid_resolutions_.size() <=4) {
      adjustBestFitLayout_Square(regionList,2);
  }else if(5<=uid_resolutions_.size() && uid_resolutions_.size() <=9) {
      adjustBestFitLayout_Square(regionList,3);
  }else if(10<=uid_resolutions_.size() && uid_resolutions_.size() <=16) {
      adjustBestFitLayout_Square(regionList,4);
  }else if(uid_resolutions_.size() ==17) {
      adjustBestFitLayout_17(regionList);
  }else {
      AG_LOG(ERROR, "adjustBestFitVideoLayout is more than 17 users");
  }
}

void RecorderEventHandler::adjustDefaultVideoLayout(agora::rtc::UserMixerLayout* regionList)
{
  auto it = uids_.begin();
  regionList[0].userId = it->c_str();
  regionList[0].config.x = 0.f;
  regionList[0].config.y = 0.f;
  regionList[0].config.width = 1.f * config_.video.width;
  regionList[0].config.height = 1.f * config_.video.height;
  regionList[0].config.zOrder = 0;

  float canvasWidth = static_cast<float>(config_.video.width);
  float canvasHeight = static_cast<float>(config_.video.height);

  float viewWidth = 0.235f;
  float viewHEdge = 0.012f;
  float viewHeight = 0.235f;
  float viewVEdge = 0.012f;

  int i = 1;
  for (++it; it != uids_.end(); ++it) {
    regionList[i].userId = it->c_str();
    float xIndex = static_cast<float>((i-1) % 4);
    float yIndex = static_cast<float>((i-1) / 4);
    regionList[i].config.x = (xIndex * (viewWidth + viewHEdge) + viewHEdge)*canvasWidth;
    regionList[i].config.y = (1 - (yIndex + 1) * (viewHeight + viewVEdge))*canvasHeight;
    regionList[i].config.width = viewWidth * canvasWidth;
    regionList[i].config.height = viewHeight * canvasHeight;
    regionList[i].config.zOrder = 1;
    i++;
  }
}

void RecorderEventHandler::setMaxResolutionUid(int number, const std::string& maxResolutionUid, agora::rtc::UserMixerLayout* regionList, double weight_ratio) 
{
  regionList[number].userId = maxResolutionUid.c_str();
  regionList[number].config.x = 0.f;
  regionList[number].config.y = 0.f;
  regionList[number].config.width = 1.f * weight_ratio * config_.video.width;
  regionList[number].config.height = 1.f * config_.video.height;
  regionList[number].config.alpha = 1.f;
}

void RecorderEventHandler::adjustVideo5Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid) 
{
  bool flag = false;
  int number = 0;
  int i = 0;
  for(const auto& uid : uids_){
    if(maxResolutionUid == uid){
        flag = true;
        setMaxResolutionUid(number,  uid, regionList,0.8);
        number++;
        continue;
    }
    regionList[number].userId = uid.c_str();
    float yIndex = flag?static_cast<float>(number-1 % 4):static_cast<float>(number % 4);
    regionList[number].config.x = 1.f * 0.8 * config_.video.width;
    regionList[number].config.y = (0.25) * yIndex * config_.video.height;
    regionList[number].config.width = 1.f*(1-0.8) * config_.video.width;
    regionList[number].config.height = 1.f * (0.25) * config_.video.height;
    regionList[number].config.alpha = 1;
    number++;
    i++;
    if(i > 4 && !flag){
        adjustVideo7Layout(regionList, maxResolutionUid);
        break;
    }
  }
}

void RecorderEventHandler::adjustVideo7Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid) 
{
  bool flag = false;
  int number = 0;
  int i = 0;
  for(const auto& uid : uids_){
    if(maxResolutionUid == uid){
      flag = true;
      setMaxResolutionUid(number,  uid, regionList,6.f/7);
      number++;
      continue;
    }
    regionList[number].userId = uid.c_str();
    float yIndex = flag?static_cast<float>(number-1 % 6):static_cast<float>(number % 6);
    regionList[number].config.x = 6.f/7 * config_.video.width;
    regionList[number].config.y = (1.f/6) * yIndex * config_.video.height;
    regionList[number].config.width = (1.f/7) * config_.video.width;
    regionList[number].config.height = (1.f/6) * config_.video.height;
    regionList[number].config.alpha = 1;
    number++;
    i++;
    if(i > 6 && !flag){
      adjustVideo9Layout(regionList, maxResolutionUid);
      break;
    }
  }
}

void RecorderEventHandler::adjustVideo9Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid) 
{
  bool flag = false;
  int number = 0;
  int i = 0;
  for(const auto& uid : uids_){
    if(maxResolutionUid == uid){
      flag = true;
      setMaxResolutionUid(number,  uid, regionList,9.f/5);
      number++;
      continue;
    }
    regionList[number].userId = uid.c_str();
    float yIndex = flag?static_cast<float>(number-1 % 8):static_cast<float>(number % 8);
    regionList[number].config.x = 8.f/9 * config_.video.width;
    regionList[number].config.y = (1.f/8) * yIndex * config_.video.height;
    regionList[number].config.width = 1.f/9 * config_.video.width;
    regionList[number].config.height = 1.f/8 * config_.video.height;
    regionList[number].config.alpha = 1;
    number++;
    i++;
    if(i > 8 && !flag){
      adjustVideo17Layout(regionList, maxResolutionUid);
      break;
    }
  }
}

void RecorderEventHandler::adjustVideo17Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid)
{
  bool flag = false;
  int number = 0;
  int i = 0;
  for(const auto& uid : uids_){
    if(maxResolutionUid == uid){
      flag = true;
      setMaxResolutionUid(number,  uid, regionList,0.8);
      number++;
      continue;
    }
    if(!flag && i == 16) {
      break;
    }
    regionList[number].userId = uid.c_str();
    float yIndex = flag?static_cast<float>((number-1) % 8):static_cast<float>(number % 8);
    regionList[number].config.x = (((flag && i>8) || (!flag && i >=8)) ? (9.f/10):(8.f/10)) * config_.video.width;
    regionList[number].config.y = ((1.f/8) * yIndex) * config_.video.height;
    regionList[number].config.width =  1.f/10 * config_.video.width;
    regionList[number].config.height = 1.f/8 * config_.video.height;
    regionList[number].config.alpha = 1;
    number++;
    i++;
  }
}

void RecorderEventHandler::adjustVerticalPresentationLayout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid) {
  if(uids_.size() <= 5) {
      adjustVideo5Layout(regionList, maxResolutionUid);
  }else if(uids_.size() <= 7) {
      adjustVideo7Layout(regionList, maxResolutionUid);
  }else if(uids_.size() <= 9) {
      adjustVideo9Layout(regionList, maxResolutionUid);
  }else {
      adjustVideo17Layout(regionList, maxResolutionUid);
  }
}

//  IAgoraMediaRtcRecorderEventHandler

void RecorderEventHandler::onConnected(const char *channelId, agora::user_id_t uid)
{
  AG_LOG(INFO, "onConnected, channelId: %s, uid: %s", channelId, uid);
}

void RecorderEventHandler::onDisconnected(const char *channelId, agora::user_id_t uid, agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
  AG_LOG(INFO, "onDisconnected, channelId: %s, uid: %s, reason: %d", channelId, uid, reason);
}

void RecorderEventHandler::onReconnected(const char *channelId, agora::user_id_t uid,agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
  AG_LOG(INFO, "onReconnected, channelId: %s, uid: %s, reason: %d", channelId, uid, reason);
}

void RecorderEventHandler::onConnectionLost(const char *channelId, agora::user_id_t uid)
{
  AG_LOG(ERROR, "onConnectionLost, channelId: %s, uid: %s", channelId, uid);
}

void RecorderEventHandler::onUserJoined(const char *channelId,agora::user_id_t uid)
{
  AG_LOG(INFO, "onUserJoined, channelId: %s, uid: %s", channelId, uid);
  
  if(config_.recorderStreamType == RECORDER_AUDIO_ONLY){
    if(!config_.SubAllAudio && 
      std::find(config_.SubAudioUserList.begin(), config_.SubAudioUserList.end(),uid) == config_.SubAudioUserList.end()){
        return;
      }
  } else if(config_.recorderStreamType == RECORDER_VIDEO_ONLY){
    if(!config_.SubAllVideo && 
      std::find(config_.SubVideoUserList.begin(), config_.SubVideoUserList.end(), uid) == config_.SubVideoUserList.end()){
        return;
      }
  } else if(config_.recorderStreamType == RECORDER_BOTH){
    if((!config_.SubAllAudio && std::find(config_.SubAudioUserList.begin(), config_.SubAudioUserList.end(),uid) == config_.SubAudioUserList.end())
      && (!config_.SubAllVideo && std::find(config_.SubVideoUserList.begin(), config_.SubVideoUserList.end(), uid) == config_.SubVideoUserList.end()))
    {
      return;
    }
  }

  {
    std::lock_guard<std::mutex> lock(mtx);
    uids_.insert(uid);
  }

  if(!config_.isMix){
    agora::media::MediaRecorderConfiguration config;
    config.fps = config_.video.fps;
    config.width = config_.video.width;
    config.height = config_.video.height;
    config.channel_num = config_.audio.numOfChannels;
    config.sample_rate = config_.audio.sampleRate;
    std::string curTime = getCurrentTimeAsString();
    std::string callID = config_.callID.c_str();
    std::string storagePath = config_.recorderPath + std::string(uid) + "_" + callID + ".mp4";
    config.storagePath = storagePath.c_str();
    config.streamType = static_cast<agora::media::MediaRecorderStreamType>(config_.recorderStreamType) ;
    config.maxDurationMs = config_.maxDuration * 1000;
    recorder_->setRecorderConfigByUid(config, uid);

    agora::rtc::WatermarkConfig watermarks[config_.waterMarks.size()];
    for(int i = 0; i < config_.waterMarks.size(); i++){
      watermarks[i].index = i+1;
      if(config_.waterMarks[i].type == Literal){
        watermarks[i].type = agora::rtc::LITERA;
        watermarks[i].literaSource.wmLitera = config_.waterMarks[i].literalMark.litera.c_str();
        watermarks[i].literaSource.fontFilePath = config_.waterMarks[i].literalMark.fontFilePath.c_str();
        watermarks[i].literaSource.fontSize = config_.waterMarks[i].literalMark.fontSize;
      } else if(config_.waterMarks[i].type == Time) {
        watermarks[i].type = agora::rtc::TIMESTAMPS;
        watermarks[i].timestampSource.fontFilePath = config_.waterMarks[i].timeMark.fontFilePath.c_str();
        watermarks[i].timestampSource.fontSize = config_.waterMarks[i].timeMark.fontSize;
      } else if(config_.waterMarks[i].type == Picture){
        watermarks[i].type = agora::rtc::PICTURE;
        watermarks[i].imageUrl = config_.waterMarks[i].pictureMark.image_url.c_str();
      }

      watermarks[i].options.mode = agora::rtc::FIT_MODE_COVER_POSITION;
      watermarks[i].options.zOrder = config_.waterMarks[i].pos.zorder;
      watermarks[i].options.positionInPortraitMode.x = config_.waterMarks[i].pos.x;
      watermarks[i].options.positionInPortraitMode.y = config_.waterMarks[i].pos.y;
      watermarks[i].options.positionInPortraitMode.width = config_.waterMarks[i].pos.width;
      watermarks[i].options.positionInPortraitMode.height = config_.waterMarks[i].pos.height;
      watermarks[i].options.positionInLandscapeMode.x = config_.waterMarks[i].pos.x;
      watermarks[i].options.positionInLandscapeMode.y = config_.waterMarks[i].pos.y;
      watermarks[i].options.positionInLandscapeMode.width = config_.waterMarks[i].pos.width;
      watermarks[i].options.positionInLandscapeMode.height = config_.waterMarks[i].pos.height;
    }
    recorder_->enableAndUpdateVideoWatermarksByUid(watermarks, config_.waterMarks.size(), uid);

    recorder_->startSingleRecordingByUid(uid);
  }
    
}

void RecorderEventHandler::onUserLeft(const char *channelId,agora::user_id_t uid, agora::rtc::USER_OFFLINE_REASON_TYPE reason)
{
  AG_LOG(INFO, "onUserLeft, channelId: %s, uid: %s, reason: %d", channelId, uid, reason);
  if(!config_.SubAllVideo && 
    std::find(config_.SubVideoUserList.begin(), config_.SubVideoUserList.end(), uid) == config_.SubVideoUserList.end()){
        return;
  }
  
  { 
    std::lock_guard<std::mutex> lock(mtx);
    uids_.erase(uid);
  }

  uid_resolutions_.erase(uid);
  if(config_.isMix){
      setVideoMixLayout();
  } else {
      recorder_->stopSingleRecordingByUid(uid);
  }
}

void RecorderEventHandler::onFirstRemoteVideoDecoded(const char *channelId,agora::user_id_t userId, int width, int height, int elapsed)
{
  AG_LOG(INFO,"onFirstRemoteVideoDecoded, channelId: %s, userId: %s, width: %d, height: %d, elapsed: %d", channelId, userId, width, height,elapsed);
  uid_resolutions_[userId] = {width, height};
  if(config_.isMix){
    setVideoMixLayout();
  } 
}

void RecorderEventHandler::onFirstRemoteAudioDecoded(const char *channelId,agora::user_id_t userId, int elapsed)
{
  AG_LOG(INFO,"onFirstRemoteAudioDecoded, channelId: %s, userId: %s, elapsed: %d", channelId, userId, elapsed);
}

void RecorderEventHandler::onRecorderStateChanged(const char *channelId, agora::user_id_t userId, agora::media::RecorderState state, agora::media::RecorderReasonCode reason, const char* filename) 
{
  AG_LOG(INFO, "onRecorderStateChanged, channelId: %s, userId: %s, state: %d, fileName: %s, reason: %d", channelId, userId, state, filename, reason);
}

void RecorderEventHandler::onRecorderInfoUpdated(const char *channelId, agora::user_id_t userId, const agora::media::RecorderInfo &info) 
{
  AG_LOG(INFO, "onRecorderInfoUpdated, channelId: %s, userId: %s, fileName: %s, duration: %d, fileSize: %d", 
    channelId, userId, info.fileName, info.durationMs, info.fileSize);
}

void RecorderEventHandler::onAudioVolumeIndication(const char *channelId, const agora::rtc::SpeakVolumeInfo* speakers,  unsigned int speakerNumber)
{
  for(int i = 0; i < speakerNumber; i++){
    AG_LOG(INFO, "onAudioVolumeIndication, channelId: %s, userId: %s, volume: %d", channelId, speakers[i].userId, speakers[i].volume);
  }
}
void RecorderEventHandler::onActiveSpeaker(const char *channelId,agora::user_id_t userId)
{
  AG_LOG(INFO, "onActiveSpeaker, channelId: %s, userId: %s", channelId, userId);
}

void RecorderEventHandler::onEncryptionError(const char* channelId, agora::rtc::ENCRYPTION_ERROR_TYPE errorType)
{
  AG_LOG(INFO, "onEncryptionError, channelId: %s, errorType: %d", channelId, errorType);
}

// IRecorderVideoFrameObserver
void RecorderVideoFrameObserver::onYuvFrameCaptured(const char* channelId, agora::user_id_t userId, const agora::media::base::VideoFrame *frame)
{
  AG_LOG(INFO, "onRecorderYuvFrameCapture: channelId=%s, userId=%s, width=%d, height=%d",channelId, userId, frame->width, frame->height);
}

void RecorderVideoFrameObserver::onEncodedFrameReceived(const char* channelId, agora::user_id_t userId, const uint8_t* imageBuffer, size_t length,agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo)
{
  AG_LOG(INFO, "onRecorderEncodedFrameCapture: channelId=%s, userId=%s, length=%zu, codecType=%d", channelId, userId, length, videoEncodedFrameInfo.codecType);
}

void RecorderVideoFrameObserver::onJPGFileSaved(const char* channelId, agora::user_id_t userId, const char* jpgFilePath) 
{
  AG_LOG(INFO, "onRecorderJpgFileCapture: channelId=%s, userId=%s, jpgFilePath=%s", channelId, userId, jpgFilePath);
}