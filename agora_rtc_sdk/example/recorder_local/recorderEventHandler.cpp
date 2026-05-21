#include "recorderEventHandler.h"
#include "IAgoraMediaRtcRecorder.h"
#include <regex>
#include <string>
#include <string>
#include "log.h"
#include <ctime>
#include "time_util.h"

// layout
void RecorderEventHandler::setVideoMixLayout()
{
    std::lock_guard<std::mutex> lock(mtx);
    
    if(uid_resolutions_.empty()) {
        AG_LOG(INFO, "setVideoMixLayout: No active video streams yet.");
        return;
    }

    agora::rtc::VideoMixingLayout layout;
    layout.canvasWidth = config_.video.width;
    layout.canvasHeight = config_.video.height;
    layout.backgroundColor = 0x000000;

    std::vector<agora::rtc::UserMixerLayout> userLayouts;

    // 1. REGION GUEST (Background - Layar Penuh, jaga aspek rasio)
    if (!config_.guestUid.empty() && config_.guestUid != "0") {
        agora::rtc::UserMixerLayout guest;
        guest.userId = config_.guestUid.c_str();

        int guestX = 0, guestY = 0;
        int guestW = config_.video.width;
        int guestH = config_.video.height;

        auto it = uid_resolutions_.find(config_.guestUid);
        if (it != uid_resolutions_.end() && it->second.width > 0 && it->second.height > 0) {
            float srcAspect    = (float)it->second.width  / it->second.height;
            float canvasAspect = (float)config_.video.width / config_.video.height;
            if (srcAspect < canvasAspect) {
                // Video lebih sempit dari canvas (misal portrait) → pillarbox
                guestH = config_.video.height;
                guestW = (int)(config_.video.height * srcAspect);
                guestX = (config_.video.width - guestW) / 2;
                guestY = 0;
            } else {
                // Video lebih lebar dari canvas → letterbox
                guestW = config_.video.width;
                guestH = (int)(config_.video.width / srcAspect);
                guestX = 0;
                guestY = (config_.video.height - guestH) / 2;
            }
        }

        guest.config.x      = guestX;
        guest.config.y      = guestY;
        guest.config.width  = guestW;
        guest.config.height = guestH;
        guest.config.zOrder = 0;
        guest.config.alpha  = 1.0;
        userLayouts.push_back(guest);
    }

    // 2. REGION HOST (Overlay - Kotak Kecil Kanan Bawah)
    if (!config_.hostUid.empty() && config_.hostUid != "0") {
        agora::rtc::UserMixerLayout host;
        host.userId = config_.hostUid.c_str();
        host.config.x = static_cast<int>(config_.video.width * 0.73);
        host.config.y = static_cast<int>(config_.video.height * 0.70);
        host.config.width = static_cast<int>(config_.video.width * 0.25);
        host.config.height = static_cast<int>(config_.video.height * 0.25);
        host.config.zOrder = 1;
        host.config.alpha = 1.0;
        userLayouts.push_back(host);
    }

    layout.userLayoutConfigNum = userLayouts.size();
    layout.userLayoutConfigs = userLayouts.data();

    int ret = recorder_->setVideoMixingLayout(layout);
    if (ret < 0) {
        AG_LOG(ERROR, "PiP Layout Update Failed: %d", ret);
    } else {
        AG_LOG(INFO, "PiP Layout Updated. Regions: %zu", userLayouts.size());
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

bool RecorderEventHandler::isSubscribeStream(agora::user_id_t uid, bool isAudio) 
{
  auto sub_match = [](const std::string &in_str, const std::string &regex) {
    const std::regex pieces_regex(regex);
    std::smatch pieces_match;
    return std::regex_match(in_str, pieces_match, pieces_regex);
  };

  const std::string *subscribeRegula = isAudio ? &config_.subscribeAudioRegula : &config_.subscribeVideoRegula;
  return sub_match(uid, *subscribeRegula);
}

void RecorderEventHandler::onUserJoined(const char *channelId,agora::user_id_t uid)
{
  AG_LOG(INFO, "onUserJoined, channelId: %s, uid: %s", channelId, uid);
  {
    std::lock_guard<std::mutex> lock(mtx);
    uids_.insert(uid);
  }

  agora::media::MediaRecorderStreamType streamType = agora::media::STREAM_TYPE_AUDIO;
  if(!config_.autoSubscribe){
    bool subAudio = false;
    bool subVideo = false;
    if(!config_.isVideoOnly && 
        (config_.subscribeAudioRegula.empty() || 
        isSubscribeStream(uid, true) || 
        config_.subscribeAudioUserList.find(uid) != config_.subscribeAudioUserList.end())){
      
      recorder_->subscribeAudio(uid);
      subAudio = true;
    }
    if(!config_.isAudioOnly && 
      (config_.subscribeVideoRegula.empty() || 
      isSubscribeStream(uid, false) || 
      config_.subscribeVideoUserList.find(uid) != config_.subscribeVideoUserList.end())){

      agora::rtc::VideoSubscriptionOptions options;
      if(config_.isMix || config_.getVideoFrame > 1){
        options.encodedFrameOnly = false;
      } else {
        options.encodedFrameOnly = true;
      }
      options.type = config_.streamType == 0 ? agora::rtc::VIDEO_STREAM_HIGH : agora::rtc::VIDEO_STREAM_LOW;
      recorder_->subscribeVideo(uid, options);
      subVideo = true;
    }
    if(!subAudio && !subVideo){
      AG_LOG(INFO, "onUserJoined, uid: %s, not subscribe audio and video", uid);
      return;
    } else if(subAudio && subVideo){
      streamType = agora::media::STREAM_TYPE_BOTH;
    } else if(subAudio){
      streamType = agora::media::STREAM_TYPE_AUDIO;
    } else {
      streamType = agora::media::STREAM_TYPE_VIDEO;
    }
  } else {
    if(config_.isAudioOnly) {
      streamType = agora::media::STREAM_TYPE_AUDIO;
    } else if(config_.isVideoOnly) {
      streamType = agora::media::STREAM_TYPE_VIDEO;
    } else {
      streamType = agora::media::STREAM_TYPE_BOTH;
    }
  }
  AG_LOG(INFO, "uid: %s, recorder streamType: %d", uid, streamType);

  if(!config_.isMix){
    agora::media::MediaRecorderConfiguration config;
    config.fps = config_.video.fps;
    config.width = config_.video.width;
    config.height = config_.video.height;
    config.channel_num = config_.audio.numOfChannels;
    config.sample_rate = config_.audio.sampleRate;
    
    char timeBuffer[80];
    Time2UTCStr(timeBuffer, 80);
    std::string storagePath = config_.recordFileRootDir + std::string(uid) + "_" + config_.callID + ".mp4";
    config.storagePath = storagePath.c_str();
    config.streamType = streamType;
    config.maxDurationMs = config_.maxDuration * 1000;
    recorder_->setRecorderConfigByUid(config, uid);
    recorder_->startSingleRecordingByUid(uid);
  }
}

void RecorderEventHandler::onUserLeft(const char *channelId,agora::user_id_t uid, agora::rtc::USER_OFFLINE_REASON_TYPE reason)
{
  AG_LOG(INFO, "onUserLeft, channelId: %s, uid: %s, reason: %d", channelId, uid, reason);
  
  {
    std::lock_guard<std::mutex> lock(mtx);
    uids_.erase(uid);
  }

  uid_resolutions_.erase(uid);
  if(config_.isMix){
    setVideoMixLayout();
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

void RecorderEventHandler::onUserVideoStateChanged(const char *channelId,agora::user_id_t userId, agora::rtc::REMOTE_VIDEO_STATE state, agora::rtc::REMOTE_VIDEO_STATE_REASON reason, int elapsed)
{
  AG_LOG(INFO, "onUserVideoStateChanged, channelId: %s, userId: %s, state: %d, reason: %d, elapsed: %d", channelId, userId, state, reason, elapsed);
}

void RecorderEventHandler::onUserAudioStateChanged(const char *channelId,agora::user_id_t userId, agora::rtc::REMOTE_AUDIO_STATE state, agora::rtc::REMOTE_AUDIO_STATE_REASON reason, int elapsed)
{
  AG_LOG(INFO, "onUserAudioStateChanged, channelId: %s, userId: %s, state: %d, reason: %d, elapsed: %d", channelId, userId, state, reason, elapsed);
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
