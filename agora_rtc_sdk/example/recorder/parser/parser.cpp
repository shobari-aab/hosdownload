#include "cJSON.h"
#include <cstring>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "parser.h"
#include "log.h"

static char *read_file_to_string(const char *filename)
{
  FILE *file = fopen(filename, "rb"); // 以二进制模式打开文件
  if (!file)
  {
    AG_LOG(ERROR, "Failed to open file: %s\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET); // 移动回文件开头

  // 分配内存以存储文件内容
  char *buffer = (char *)malloc(file_size + 1); // +1 为了存储字符串结束符
  if (!buffer)
  {
    AG_LOG(ERROR, "Failed to allocate memory");
    fclose(file);
    return NULL;
  }

  // 读取文件内容
  fread(buffer, 1, file_size, file);
  buffer[file_size] = '\0'; // 添加字符串结束符

  fclose(file);
  return buffer;
}

void printRecorderConfig(const recorder_config &config)
{
  AG_LOG(INFO, "RecorderConfig:");
  AG_LOG(INFO, "appid: %s, token: %s, channelName: %s UserId: %s", config.appId.c_str(), config.token.c_str(), config.ChannelName.c_str(), config.UserId.c_str());
  AG_LOG(INFO, "audio: sampleRate: %d, numofChannels; %d", config.audio.sampleRate, config.audio.numOfChannels);
  AG_LOG(INFO, "video: width: %d, height: %d,  fps: %d", config.video.width, config.video.height, config.video.fps);
  AG_LOG(INFO, "useStringUid: %d, useCloudProxy: %d, useLocalAp: %d", config.UseStringUid, config.UseCloudProxy, config.UseLocalAp);
  if(config.UseLocalAp){
    AG_LOG(INFO, "localAp: mode: %d, verifyDomainName: %s", config.localAp.mode, config.localAp.verifyDomainName.c_str());
    AG_LOG(INFO, "domainList: ");
    for(const auto& it : config.localAp.domainList){
      AG_LOG(INFO, " %s ", it.c_str());
    }
    AG_LOG(INFO, "ipList: ");
    for(const auto& it : config.localAp.ipList){
      AG_LOG(INFO, " %s ", it.c_str());
    }
  }
  AG_LOG(INFO, "SubAllAudio: %d", config.SubAllAudio);
  AG_LOG(INFO, "audioVolumeIndicationIntervalMs: %d", config.audioVolumeIndicationIntervalMs);
  if(!config.SubAllAudio){
    AG_LOG(INFO, "SubAudioUserList: ");
    for(const auto& it : config.SubAudioUserList){
      AG_LOG(INFO, " %s ", it.c_str());
    }
  }
  AG_LOG(INFO, "SubAllVideo: %d, subType: %d", config.SubAllVideo, config.subStreamType);
  if(!config.SubAllVideo){
    AG_LOG(INFO, "SubVideoUserList: ");
    for(const auto& it : config.SubVideoUserList){
      AG_LOG(INFO, " %s ", it.c_str());
    }
  }
  if(config.isMix){
    AG_LOG(INFO, "isMix: %d, layoutMode: %d", config.isMix, config.layoutMode);
    AG_LOG(INFO, "backgroundColor: %d, backgroundImage: %s", config.backgroundColor, config.backgroundImage.c_str());
    for(auto& it : config.waterMarks){
      if(it.type == Literal){
        AG_LOG(INFO, "litera waterMark, litera: %s, fontFilePath: %s, fontSize: %d",
            it.literalMark.litera.c_str(), it.literalMark.fontFilePath.c_str(), it.literalMark.fontSize);
      } else if(it.type == Time){
        AG_LOG(INFO, "time waterMark fontFilePath: %s, fontSize: %d", 
          it.timeMark.fontFilePath.c_str(), it.timeMark.fontSize);
      } else if(it.type == Picture){
        AG_LOG(INFO, "picture waterMark imgUrl: %s", it.pictureMark.image_url.c_str());
      }
      AG_LOG(INFO, " pos x: %d, y: %d, width: %d, height: %d, zorder: %d", 
        it.pos.x, it.pos.y, it.pos.width, it.pos.height, it.pos.zorder);
    }
  } else {
    AG_LOG(INFO, "isMix: %d", config.isMix);
  }
  AG_LOG(INFO, "recorderStreamType: %d, recorderPaht: %s, maxDuration: %d(s), recorverFile: %d", 
    config.recorderStreamType, config.recorderPath.c_str(), config.maxDuration, config.recoverFile);
  if(config.enableEncryption){
      AG_LOG(INFO, "encryption: mode: %d, key: %s, salt: %s", config.encryption.mode, config.encryption.key.c_str(), config.encryption.salt.c_str());
  }

  for(auto& ro : config.rotationMap){
    AG_LOG(INFO, "rotation uid: %s, degree: %d", ro.first.c_str(), ro.second);
  }
  
  AG_LOG(INFO, "frameCaptureConfig: enable: %d, videoFrameType: %d, jpgCaptureInterval: %d, jpgFileStorePath: %s",
    config.frameCaptureConfig.enable, config.frameCaptureConfig.videoFrameType,
    config.frameCaptureConfig.jpgCaptureInterval, config.frameCaptureConfig.jpgFileStorePath.c_str());
}

void parseLocalAp(cJSON *localAp_json, recorder_config& config)
{
  cJSON *mode = cJSON_GetObjectItem(localAp_json, "mode");
  if(cJSON_IsNumber(mode)){
    config.localAp.mode = static_cast<LOCAL_PROXY_MODE>(mode->valueint);
  }

  cJSON *domainList = cJSON_GetObjectItem(localAp_json, "domainList");
  if(cJSON_IsArray(domainList)){
    int list_size = cJSON_GetArraySize(domainList);
    for (int i = 0; i < list_size; i++)
    {
        config.localAp.domainList.push_back(cJSON_GetArrayItem(domainList, i)->valuestring);
    }
  }

  cJSON *ipList = cJSON_GetObjectItem(localAp_json, "ipList");
  if(cJSON_IsArray(ipList)){
    int list_size = cJSON_GetArraySize(ipList);
    for (int i = 0; i < list_size; i++)
    {
        config.localAp.ipList.push_back(cJSON_GetArrayItem(ipList, i)->valuestring);
    }
  }

  cJSON *verifyDomainName = cJSON_GetObjectItem(localAp_json, "verifyDomainName");
  if(cJSON_IsString(verifyDomainName)){
    config.localAp.verifyDomainName = verifyDomainName->valuestring;
  }
}

void parseBasicConfig(cJSON *config_Json, recorder_config& config)
{
  cJSON *appId = cJSON_GetObjectItem(config_Json, "appid");
  if (cJSON_IsString(appId))
  {
    config.appId = appId->valuestring;
  }
  cJSON *token = cJSON_GetObjectItem(config_Json, "token");
  if (cJSON_IsString(token)){
    config.token = token->valuestring;
  }
  cJSON *ChannelName = cJSON_GetObjectItem(config_Json, "channelName");
  if (cJSON_IsString(ChannelName))
  {
    config.ChannelName = ChannelName->valuestring;
  }
  cJSON *UserId = cJSON_GetObjectItem(config_Json, "userId");
  if (cJSON_IsString(UserId))
  {
    config.UserId = UserId->valuestring;
  }
  cJSON *UseStringUid = cJSON_GetObjectItem(config_Json, "useStringUid");
  if (cJSON_IsBool(UseStringUid))
  {
    config.UseStringUid = cJSON_IsTrue(UseStringUid);
  }
  cJSON *UseCloudProxy = cJSON_GetObjectItem(config_Json, "useCloudProxy");
  if (cJSON_IsBool(UseCloudProxy))
  {
    config.UseCloudProxy = cJSON_IsTrue(UseCloudProxy);
  }

  cJSON *localAp = cJSON_GetObjectItem(config_Json, "localAp");
  if(cJSON_IsObject(localAp)){
    config.UseLocalAp = true;
    parseLocalAp(localAp, config);
  } else {
    config.UseLocalAp = false;
  }
  
  cJSON *maxDuration = cJSON_GetObjectItem(config_Json, "maxDuration");
  if(cJSON_IsNumber(maxDuration)){
    config.maxDuration = maxDuration->valueint;
  }
  cJSON *audioVolumeIndicationIntervalMs = cJSON_GetObjectItem(config_Json, "audioVolumeIndicationIntervalMs");
  if(cJSON_IsNumber(audioVolumeIndicationIntervalMs)){
    config.audioVolumeIndicationIntervalMs = audioVolumeIndicationIntervalMs->valueint;
  }
}

void parseSubConfig(cJSON *config_Json, recorder_config& config)
{
  cJSON *SubAllAudio = cJSON_GetObjectItem(config_Json, "subAllAudio");
  if (cJSON_IsBool(SubAllAudio))
  {
    config.SubAllAudio = cJSON_IsTrue(SubAllAudio);
  }
  if(!config.SubAllAudio){
    cJSON *SubAudioUserList = cJSON_GetObjectItem(config_Json, "subAudioUserList");
    if (cJSON_IsArray(SubAudioUserList))
    {
      int list_size = cJSON_GetArraySize(SubAudioUserList);
      for (int i = 0; i < list_size; i++)
      {
          config.SubAudioUserList.push_back(cJSON_GetArrayItem(SubAudioUserList, i)->valuestring);
      }
    }
  }
  
  cJSON *SubAllVideo = cJSON_GetObjectItem(config_Json, "subAllVideo");
  if (cJSON_IsBool(SubAllVideo))
  {
    config.SubAllVideo = cJSON_IsTrue(SubAllVideo);
  }
  if (!config.SubAllVideo){
    cJSON *SubVideoUserList = cJSON_GetObjectItem(config_Json, "subVideoUserList");
    if (cJSON_IsArray(SubVideoUserList))
    {
      int list_size = cJSON_GetArraySize(SubVideoUserList);
      for (int i = 0; i < list_size; i++)
      {
          config.SubVideoUserList.push_back(cJSON_GetArrayItem(SubVideoUserList, i)->valuestring);
      }
    }
  }
  cJSON *subStreamType = cJSON_GetObjectItem(config_Json, "subStreamType");
  if(cJSON_IsString(subStreamType)){
    if(0 == strcmp(subStreamType->valuestring, "high")){
      config.subStreamType = STREAM_HIGH;
    } else if (0 == strcmp(subStreamType->valuestring, "low")){
      config.subStreamType = STREAM_LOW;
    } else {
      AG_LOG(ERROR, "wrong subStreamType: %s", subStreamType->valuestring);
    }
  }
}

void parseMixerConfig(cJSON *config_Json, recorder_config& config)
{
  cJSON *isMix = cJSON_GetObjectItem(config_Json, "isMix");
  if(cJSON_IsBool(isMix)){
    config.isMix = cJSON_IsTrue(isMix);
  }
  if(config.isMix){
    cJSON *layoutMode = cJSON_GetObjectItem(config_Json, "layoutMode");
    if(cJSON_IsString(layoutMode)){
      if(0 == strcmp(layoutMode->valuestring, "default")){
        config.layoutMode = DEFAULT_LAYOUT;
      } else if(0 == strcmp(layoutMode->valuestring, "bestfit")){
        config.layoutMode = BESTFIT_LAYOUT;
      } else if( 0 == strcmp(layoutMode->valuestring, "vertical")){
        config.layoutMode = VERTICALPRESENTATION_LAYOUT;
      } else {
        AG_LOG(ERROR, "wrong layoutMode: %s", layoutMode->valuestring);
      }
    }

    cJSON *maxResolutionUid = cJSON_GetObjectItem(config_Json, "maxResolutionUid");
    if(cJSON_IsString(maxResolutionUid)){
      config.maxResolutionUid = maxResolutionUid->valuestring;
    }

    cJSON *backgroundColor = cJSON_GetObjectItem(config_Json, "backgroundColor");
    if(cJSON_IsNumber(backgroundColor)){
      config.backgroundColor = backgroundColor->valueint;
    }

    cJSON *backgroundImage = cJSON_GetObjectItem(config_Json, "backgroundImage");
    if(cJSON_IsString(backgroundImage)){
      config.backgroundImage = backgroundImage->valuestring;
    }
  }
}

void parseRecordConfig(cJSON *config_Json, recorder_config& config)
{
  cJSON *recorderStreamType = cJSON_GetObjectItem(config_Json, "recorderStreamType");
  if(cJSON_IsString(recorderStreamType)){
    if(0 == strcmp(recorderStreamType->valuestring, "audio_only")){
      config.recorderStreamType = RECORDER_AUDIO_ONLY;
    } else if(0 == strcmp(recorderStreamType->valuestring, "video_only")) {
      config.recorderStreamType = RECORDER_VIDEO_ONLY;
    } else if(0 == strcmp(recorderStreamType->valuestring, "both")) {
      config.recorderStreamType = RECORDER_BOTH;
    } else {
      AG_LOG(ERROR, "wrong recorderStreamType: %s", recorderStreamType->valuestring);
    }
  }

  cJSON *recorderPath = cJSON_GetObjectItem(config_Json, "recorderPath");
  if (cJSON_IsString(recorderPath)){
    config.recorderPath = recorderPath->valuestring;
  }

  cJSON *callID = cJSON_GetObjectItem(config_Json, "callID");
  if (cJSON_IsString(callID)){
    config.callID = callID->valuestring;
  }

  cJSON *recoverFile = cJSON_GetObjectItem(config_Json, "recoverFile");
  if(cJSON_IsBool(recoverFile)){
    config.recoverFile = cJSON_IsTrue(recoverFile);
  }
}

void parseAudioJson(cJSON *config_Json, recorder_config& config)
{
  cJSON *audio = cJSON_GetObjectItem(config_Json, "audio");
  if(cJSON_IsObject(audio)){
    cJSON *sampleRate = cJSON_GetObjectItem(audio, "sampleRate");
    if(cJSON_IsNumber(sampleRate)){
      config.audio.sampleRate = sampleRate->valueint;
    }

    cJSON *numOfChannels = cJSON_GetObjectItem(audio, "numOfChannels");
    if(cJSON_IsNumber(numOfChannels)){
      config.audio.numOfChannels = numOfChannels->valueint;
    }
  }
}

void parseVideoJson(cJSON *config_Json, recorder_config& config)
{
  cJSON *video = cJSON_GetObjectItem(config_Json, "video");
  if(cJSON_IsObject(video)){
    cJSON *width = cJSON_GetObjectItem(video, "width");
    if(cJSON_IsNumber(width)){
      config.video.width = width->valueint;
    }

    cJSON *height = cJSON_GetObjectItem(video, "height");
    if(cJSON_IsNumber(height)){
      config.video.height = height->valueint;
    }

    cJSON *fps = cJSON_GetObjectItem(video, "fps");
    if(cJSON_IsNumber(fps)){
      config.video.fps = fps->valueint;
    }
  }
}

void getWaterMarkPos(cJSON *waterMark, waterMarkPos& pos)
{
    cJSON *x = cJSON_GetObjectItem(waterMark, "x");
    if(cJSON_IsNumber(x)){
        pos.x = x->valueint;
    }

    cJSON *y = cJSON_GetObjectItem(waterMark, "y");
    if(cJSON_IsNumber(y)){
        pos.y = y->valueint;
    }

    cJSON *width = cJSON_GetObjectItem(waterMark, "width");
    if(cJSON_IsNumber(width)){
        pos.width = width->valueint;
    }

    cJSON *height = cJSON_GetObjectItem(waterMark, "height");
    if(cJSON_IsNumber(height)){
        pos.height = height->valueint;
    }

    cJSON *zorder = cJSON_GetObjectItem(waterMark, "zorder");
    if(cJSON_IsNumber(zorder)){
        pos.zorder = zorder->valueint;
    }

}

void parseLiteraWaterMark(cJSON *literaWaterMark, WaterMark& mark)
{
    cJSON *litera = cJSON_GetObjectItem(literaWaterMark, "litera");
    if(cJSON_IsString(litera)){
        mark.literalMark.litera = litera->valuestring;
    }

    cJSON *fontFilePath = cJSON_GetObjectItem(literaWaterMark, "fontFilePath");
    if(cJSON_IsString(fontFilePath)){
        mark.literalMark.fontFilePath = fontFilePath->valuestring;
    }

    cJSON *fontSize = cJSON_GetObjectItem(literaWaterMark, "fontSize");
    if(cJSON_IsNumber(fontSize)){
        mark.literalMark.fontSize = fontSize->valueint;
    }
}

void parseTimeWaterMark(cJSON *timeWaterMark, WaterMark& mark)
{
    cJSON *fontFilePath = cJSON_GetObjectItem(timeWaterMark, "fontFilePath");
    if(cJSON_IsString(fontFilePath)){
        mark.timeMark.fontFilePath = fontFilePath->valuestring;
    }

    cJSON *fontSize = cJSON_GetObjectItem(timeWaterMark, "fontSize");
    if(cJSON_IsNumber(fontSize)){
        mark.timeMark.fontSize = fontSize->valueint;
    }
}

void parsePictureWaterMark(cJSON *picWaterMark, WaterMark& mark)
{
    cJSON *imgUrl = cJSON_GetObjectItem(picWaterMark, "imgUrl");
    if(cJSON_IsString(imgUrl)){
        mark.pictureMark.image_url = imgUrl->valuestring;
    }
}

void parseWaterJson(cJSON *config_Json, recorder_config& config)
{
  cJSON *WaterMarks = cJSON_GetObjectItem(config_Json, "waterMark");
  if(cJSON_IsArray(WaterMarks)){
    int size = cJSON_GetArraySize(WaterMarks);
    for(int i = 0; i < size; i++){
      cJSON *item = cJSON_GetArrayItem(WaterMarks, i);
      if(cJSON_IsObject(item)){
        WaterMark mark;
        cJSON *type = cJSON_GetObjectItem(item, "type");
        if(cJSON_IsString(type)){
          if(0 == strcmp(type->valuestring, "litera")){
            mark.type = Literal;
            parseLiteraWaterMark(item, mark);
          } else if(0 == strcmp(type->valuestring, "time")){
            mark.type = Time;
            parseTimeWaterMark(item, mark);
          } else if(0 == strcmp(type->valuestring, "picture")){
            mark.type = Picture;
            parsePictureWaterMark(item, mark);
          } else {
            continue;
          }
        }
        getWaterMarkPos(item, mark.pos);
        config.waterMarks.push_back(mark);
      }
    }
  }
}

ENCRYPTION_MODE getEncryptionMode(const char* mode)
{
    if(0 == strcmp(mode, "AES_128_XTS")){
        return AES_128_XTS;
    } else if(0 == strcmp(mode, "AES_128_ECB")){
        return AES_128_ECB;
    } else if(0 == strcmp(mode, "AES_256_XTS")){
        return AES_256_XTS;
    } else if(0 == strcmp(mode, "SM4_128_ECB")){
        return SM4_128_ECB;
    } else if(0 == strcmp(mode, "AES_128_GCM")){
        return AES_128_GCM;
    } else if(0 == strcmp(mode, "AES_256_GCM")){
        return AES_256_GCM;
    } else if(0 == strcmp(mode, "AES_128_GCM2")){
        return AES_128_GCM2;
    } else if(0 == strcmp(mode, "AES_256_GCM2")) {
        return AES_256_GCM2;
    }
    return MODE_END;
}

void parseEncryptionJson(cJSON *config_Json, recorder_config& config)
{
  cJSON *encryption = cJSON_GetObjectItem(config_Json, "encryption");
  if(cJSON_IsObject(encryption)){
    config.enableEncryption = true;
    cJSON *mode = cJSON_GetObjectItem(encryption, "mode");
    if(cJSON_IsString(mode)){
        config.encryption.mode = getEncryptionMode(mode->valuestring);
    }

    cJSON *key = cJSON_GetObjectItem(encryption, "key");
    if(cJSON_IsString(key)){
        config.encryption.key = key->valuestring;
    }

    cJSON *salt = cJSON_GetObjectItem(encryption, "salt");
    if(cJSON_IsString(salt)){
        config.encryption.salt = salt->valuestring;
    }
  }
}

void parseRotationJson(cJSON *config_Json, recorder_config& config)
{
  cJSON *rotation = cJSON_GetObjectItem(config_Json, "rotation");
  if(cJSON_IsArray(rotation)){
    int size = cJSON_GetArraySize(rotation);
    for(int i = 0; i < size; i++){
      cJSON *item = cJSON_GetArrayItem(rotation, i);
      if(cJSON_IsObject(item)){
        cJSON *uid = cJSON_GetObjectItem(item, "uid");
        cJSON *degree = cJSON_GetObjectItem(item, "degree");
        if(cJSON_IsString(uid) && cJSON_IsNumber(degree)){
            config.rotationMap[uid->valuestring] = degree->valueint;
        }
      }
    }
  }
}

void parseVideoFrameCaptureJson(cJSON *config_Json, recorder_config& config)
{
  cJSON *frameCapture = cJSON_GetObjectItem(config_Json, "frameCapture");
  if(cJSON_IsObject(frameCapture)){
    config.frameCaptureConfig.enable = true;
    cJSON *videoFrameType = cJSON_GetObjectItem(frameCapture, "videoFrameType");
    if(cJSON_IsString(videoFrameType)){
      if(0 == strcmp(videoFrameType->valuestring, "encoded_frame")){
        config.frameCaptureConfig.videoFrameType = 0;
      } else if(0 == strcmp(videoFrameType->valuestring, "yuv_frame")){
        config.frameCaptureConfig.videoFrameType = 1;
      } else if(0 == strcmp(videoFrameType->valuestring, "jpg_frame")){
        config.frameCaptureConfig.videoFrameType = 2;
      } else if(0 == strcmp(videoFrameType->valuestring, "jpg_file")){
        config.frameCaptureConfig.videoFrameType = 3;
      } else {
        AG_LOG(ERROR, "wrong videoFrameType: %s", videoFrameType->valuestring);
      }
    }

    cJSON *jpgCaptureInterval = cJSON_GetObjectItem(frameCapture, "jpgCaptureInterval");
    if(cJSON_IsNumber(jpgCaptureInterval)){
      config.frameCaptureConfig.jpgCaptureInterval = jpgCaptureInterval->valueint;
    }

    cJSON *jpgFileStorePath = cJSON_GetObjectItem(frameCapture, "jpgFileStorePath");
    if(cJSON_IsString(jpgFileStorePath)){
      config.frameCaptureConfig.jpgFileStorePath = jpgFileStorePath->valuestring;
    }
  }
}

recorder_config getRecorderConfigbyJson(const char *filename)
{
  recorder_config config;

  char *json_buffer = read_file_to_string(filename);
  if (json_buffer == NULL){
    AG_LOG(ERROR, "using default recorder config");
    return config;
  }
  cJSON *config_Json = cJSON_Parse(json_buffer);
  
  parseBasicConfig(config_Json, config);
  parseSubConfig(config_Json, config);
  parseMixerConfig(config_Json, config);
  parseRecordConfig(config_Json, config);
  parseAudioJson(config_Json, config);
  parseVideoJson(config_Json, config);
  parseWaterJson(config_Json, config);
  parseEncryptionJson(config_Json, config);
  parseRotationJson(config_Json, config);
  parseVideoFrameCaptureJson(config_Json, config);

  free(json_buffer);
  cJSON_Delete(config_Json);
  return config;
}