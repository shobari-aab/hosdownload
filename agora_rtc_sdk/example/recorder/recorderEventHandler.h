#pragma once

#include "IAgoraMediaRtcRecorderEventHandler.h"
#include "IAgoraMediaRtcRecorder.h"
#include "parser/parser.h"
#include <mutex>
#include <set>
#include <string>

class RecorderEventHandler : public agora::rtc::IAgoraMediaRtcRecorderEventHandler{
  public:
    RecorderEventHandler(agora::agora_refptr<agora::rtc::IAgoraMediaRtcRecorder> recorder, recorder_config& config)
    :recorder_(recorder), config_(config){};
    virtual ~RecorderEventHandler() {};

    std::set<std::string> getUids(){
      std::lock_guard<std::mutex> lock(mtx);
      return uids_;
    }
    //  IAgoraMediaRtcRecorderEventHandler
    void onConnected(const char *channelId, agora::user_id_t uid);
    void onDisconnected(const char *channelId, agora::user_id_t uid, agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason);
    void onReconnected(const char *channelId, agora::user_id_t uid,agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason);
    void onConnectionLost(const char *channelId, agora::user_id_t uid);
    void onUserJoined(const char *channelId,agora::user_id_t uid);
    void onUserLeft(const char *channelId,agora::user_id_t uid, agora::rtc::USER_OFFLINE_REASON_TYPE reason);
    void onFirstRemoteVideoDecoded(const char *channelId,agora::user_id_t userId, int width, int height, int elapsed);
    void onFirstRemoteAudioDecoded(const char *channelId,agora::user_id_t userId, int elapsed);
    void onAudioVolumeIndication(const char *channelId, const agora::rtc::SpeakVolumeInfo* speakers,  unsigned int speakerNumber);
    void onActiveSpeaker(const char *channelId,agora::user_id_t userId);
    void onUserVideoStateChanged(const char *channelId,agora::user_id_t userId, agora::rtc::REMOTE_VIDEO_STATE state, agora::rtc::REMOTE_VIDEO_STATE_REASON reason, int elapsed){}; 
    void onUserAudioStateChanged(const char *channelId,agora::user_id_t userId, agora::rtc::REMOTE_AUDIO_STATE state, agora::rtc::REMOTE_AUDIO_STATE_REASON reason, int elapsed){}; 
    void onRemoteVideoStats(const char *channelId, agora::user_id_t userId, const agora::rtc::RemoteVideoStatistics& stats){};
    void onRemoteAudioStats(const char *channelId, agora::user_id_t userId, const agora::rtc::RemoteAudioStatistics& stats){}; 
    void onRecorderStateChanged(const char *channelId, agora::user_id_t userId, agora::media::RecorderState state, agora::media::RecorderReasonCode reason, const char* filename);
    void onRecorderInfoUpdated(const char *channelId, agora::user_id_t userId, const agora::media::RecorderInfo &info);
    void onEncryptionError(const char* channelId, agora::rtc::ENCRYPTION_ERROR_TYPE errorType);
    void onError(const char* channelId, agora::ERROR_CODE_TYPE error, const char* msg){};
    void onTokenPrivilegeWillExpire(const char* channelId, const char* token){};
    void onTokenPrivilegeDidExpire(const char* channelId){};

  private:
    // layout
    void setVideoMixLayout();
    void adjustBestFitVideoLayout(agora::rtc::UserMixerLayout* regionList);
    void adjustBestFitLayout_Square(agora::rtc::UserMixerLayout* regionList, int nSquare);
    void adjustBestFitLayout_2(agora::rtc::UserMixerLayout* regionList);
    void adjustBestFitLayout_17(agora::rtc::UserMixerLayout* regionList);

    void adjustDefaultVideoLayout(agora::rtc::UserMixerLayout* regionList);

    void adjustVerticalPresentationLayout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid);
    void adjustVideo5Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid);
    void setMaxResolutionUid(int number, const std::string& maxResolutionUid, agora::rtc::UserMixerLayout* regionList, double weight_ratio);
    void adjustVideo7Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid);
    void adjustVideo9Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid);
    void adjustVideo17Layout(agora::rtc::UserMixerLayout* regionList, std::string maxResolutionUid);

private:
    agora::agora_refptr<agora::rtc::IAgoraMediaRtcRecorder> recorder_;
    recorder_config config_;
    std::mutex mtx;
    std::set<std::string> uids_;
    struct Resolution {
      int width;
      int height;
    };
    std::unordered_map<std::string, Resolution> uid_resolutions_;
};

class RecorderVideoFrameObserver : public agora::rtc::IRecorderVideoFrameObserver{
public:
  ~RecorderVideoFrameObserver() {}
  void onYuvFrameCaptured(const char* channelId, agora::user_id_t userId, const agora::media::base::VideoFrame *frame) override;
  void onEncodedFrameReceived(const char* channelId, agora::user_id_t userId, const uint8_t* imageBuffer, size_t length,
     agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo) override;
  void onJPGFileSaved(const char* channelId, agora::user_id_t userId, const char* jpgFilePath) override;
};