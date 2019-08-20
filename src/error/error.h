#ifndef _AVPLAYERWIDGET_ERROR_H_
#define _AVPLAYERWIDGET_ERROR_H_

#include "avplayerwidget_global.h"

#ifdef _WIN32
#pragma warning(disable: 4996)
#pragma warning(disable: 4576)
#endif /* _WIN32 */

/////////////////////////
#if defined(__MACOSX__) && defined(__LINUX__)
#define _DEBUG
#endif

#define GOTO_FAIL(err_code) {ret = KERROR(err_code); goto fail;}

#define KERROR(err) (-err)

/* error code for player widget*/
#define KESUCCESS                       0x00
#define KENOMEM                         0x01
#define KEAGAIN                         0x02
#define KEINVAL                         0x03
#define KEUNINITED                      0x04
#define KEREINIT                        0x05
#define KEEOF                           0x06
#define KEABORTED                       0x07
#define KEPLAY_OVER                     0x08
#define KEUNDEF6                        0x09
#define KEUNDEF5                        0x0A
#define KEUNDEF4                        0x0B
#define KEUNDEF3                        0x0C
#define KEUNDEF2                        0x0D
#define KEUNDEF1                        0x0E
#define KEUNDEF0                        0x0F
#define KECREATE_SDL_MUTEX_FAIL         0x10
#define KECREATE_SDL_COND_FAIL          0x11
#define KESDL_INIT_FAIL                 0x12
#define KERENDER_INIT_FAIL              0x13
#define KECREATE_SDL_WINDOW_FAIL        0x14
#define KEUNSUPPORTED_MEDIA_STREAM_TYPE 0x15
#define KECREATE_SDL_RENDERER_FAIL      0x16
#define KEGET_SDL_RENDERER_INFO_FAIL    0x17
#define KEOPEN_MEDIA_FILE_FAIL          0x18
#define KECREATE_THREAD_FAIL            0x19
#define KEFIND_STREAM_INFO_FAIL         0x1A
#define KEOPEN_INPUT_FAIL               0x1B
#define KENOAVST                        0x1C
#define KEDECODER_INIT_FAIL             0x1D
#define KECOPY_CODEC_PARAMS_FAIL        0x1E
#define KEAVCODEC_FIND_DECODER_FAIL     0x1F
#define KEOPEN_DECODER_FAIL             0x20
#define KEDEMUX_INIT_FAIL               0x21
#define KEREAD_PACKET_FAIL              0x22
#define KERECEIVE_FRAME_FAIL            0x23
#define KESEND_PACKET_FAIL              0x24
#define KEDECODE_PACKETS_FAIL           0x25
#define KEQUEUE_INIT_FAIL               0x26
#define KEWRONG_AUDIO_PARAMS            0x27
#define KEDEV_INIT_FAIL                 0x28
#define KEUNSUPPORTED_AUDIO_DATA_FORMAT 0x29
#define KEUNSUPPORTED_AUDIO_CHANNEL     0x2A
#define KEUNSUPPORTED_AUDIO_FREQ        0x2B
#define KESWR_INIT_FAIL                 0x2C
#define KESWR_CONVERT_FAIL              0x2D
#define KERESAMPLE_FAIL                 0x2E
#define KESWS_ALLOC_FAIL                0x3F
#define KESWS_SCALE_FAIL                0x30
#define KEUPLOAD_TEXTURE_FAIL           0x31
#define KERENDER_FRAME_FAIL             0x32
#define KEVIDEO_IMAGE_DISPLAY_FAIL      0x33
#define KEREALLOC_TEXTURE_FAIL          0x34
#define KEREALLOC_SWS_FAIL              0x35
#define KEUNSUPPORTED_PIXFORMAT         0x36
#define KECREATE_TEXTURE_FAIL           0x37
#define KESET_TEXTURE_BLEND_MODE_FAIL   0x38
#define KESEEK_FAIL                     0x39
#define KEAV_IMAGE_FILL_ARRAY_FAIL      0x3A
#define KEUPDATE_VIDEO_FAIL             0x3B
#define KEUPDATE_TEXTURE_FAIL           0x3C
#define KEAUDIO_FILL_FAIL               0x3D
#define KEOPEN_FONT_FAIL                0x3E
#define KECREATE_SDL_SURFACE_FAIL       0x3F
#define KEMSGER_INIT_FAIL               0x40
#define KETTF_INIT_FAIL                 0x41
#define KERENDER_MSG_FAIL               0x42
#define KENO_FIRST_FRAME                0x43
#define KELOG_FILE_OPEN_FAIL            0x44
#define KEUNDEF17                       0x45
#define KEUNDEF16                       0x46
#define KEUNDEF15                       0x47
#define KEUNDEF14                       0x48
#define KEUNDEF13                       0x49
#define KEUNDEF12                       0x4A
#define KEUNDEF11                       0x4B
#define KEUNDEF10                       0x4C
#define KEUNDEF9                        0x4D
#define KEUNDEF8                        0x4E
#define KEUNDEF7                        0x4F

/* error code for player GUI */
#define KEGUI_WINDOW_INIT_FAIL          0x50
#define KEGUI_LOG_INIT_FAIL             0x51
#define KEGUI_VIDEO_WIDGET_INIT_FAIL    0x52
#define KERROR_MAX                      KEGUI_VIDEO_WIDGET_INIT_FAIL

#define _New   new(std::nothrow)

static const char *err_string_map[] = {
    "access",                               // KESUCCESS                       
    "out of memory",                        // KENOMEM                         
    "try again",                            // KEAGAIN                         
    "invalid arguments",                    // KEINVAL                         
    "the component is uninited",            // KEUNINITED                      
    "the component have been inited"        // KEREINIT
    "end of file",                          // KEEOF
    "aborteded",                            // KEABORTED
    "play over",                            // KEPLAY_OVER
    "undefined error code",                 // KEUNDEF6                        
    "undefined error code",                 // KEUNDEF5                        
    "undefined error code",                 // KEUNDEF4                        
    "undefined error code",                 // KEUNDEF3                        
    "undefined error code",                 // KEUNDEF2                        
    "undefined error code",                 // KEUNDEF1                        
    "undefined error code",                 // KEUNDEF0                        
    "SDL mutex creation failed",            // KECREATE_SDL_MUTEX_FAIL         
    "SDL cond creation failed",             // KECREATE_SDL_COND_FAIL          
    "SDL initialization failed",            // KESDL_INIT_FAIL                 
    "render initialization failed",         // KERENDER_INIT_FAIL              
    "SDL window creation failed",           // KECREATE_SDL_WINDOW_FAIL        
    "unsupported media stream type",        // KEUNSUPPORTED_MEDIA_STREAM_TYPE 
    "SDL renderer creation failed",         // KECREATE_SDL_RENDERER_FAIL      
    "get SDL rendererer information failed",// KEGET_SDL_RENDERER_INFO_FAIL    
    "open meida file failed",               // KEOPEN_MEDIA_FILE_FAIL          
    "thread creation failed",               // KECREATE_THREAD_FAIL            
    "find stream information failed",       // KEFIND_STREAM_INFO_FAIL
    "open input file failed",               // KEOPEN_INPUT_FAIL               
    "no any audio stream and video stream", // KENOAVST                        
    "init decoder failed",                  // KEDECODER_INIT_FAIL             
    "failed to copy codec parameters",      // KECOPY_CODEC_PARAMS_FAIL        
    "unable to find decoder",               // KEAVCODEC_FIND_DECODER_FAIL     
    "to open decoder failed",               // KEOPEN_DECODER_FAIL             
    "init demux failed",                    // KEDEMUX_INIT_FAIL               
    "packet read failed",                   // KEREAD_PACKET_FAIL              
    "failed to receive frame from avcodec", // KERECEIVE_FRAME_FAIL            
    "failed to send frame to avcodec",      // KESEND_PACKET_FAIL              
    "decoding failure",                     // KEDECODE_PACKETS_FAIL           
    "queues initialization failed",         // KEQUEUE_INIT_FAIL               
    "wrong audio paramaters",               // KEWRONG_AUDIO_PARAMS            
    "device initialization failed",         // KEDEV_INIT_FAIL                 
    "unsupported audio data format",        // KEUNSUPPORTED_AUDIO_DATA_FORMAT      
    "unsupported audio channel",            // KEUNSUPPORTED_AUDIO_CHANNEL     
    "unsupported audio frequency",          // KEUNSUPPORTED_AUDIO_FREQ        
    "swr init failed",                      // KESWR_INIT_FAIL                 
    "swr convert failed",                   // KESWR_CONVERT_FAIL              
    "audio resample failed",                // KERESAMPLE_FAIL                 
    "sws allocation failed",                // KESWS_ALLOC_FAIL                
    "sws scale failed",                     // KESWS_SCALE_FAIL                
    "texture upload failed",                // KEUPLOAD_TEXTURE_FAIL           
    "render frame failed",                  // KERENDER_FRAME_FAIL             
    "video image display failed",           // KEVIDEO_IMAGE_DISPLAY_FAIL      
    "texture realloc failed",               // KEREALLOC_TEXTURE_FAIL          
    "sws realloc failed",                   // KEREALLOC_SWS_FAIL              
    "unsupported pixel format",             // KEUNSUPPORTED_PIXFORMAT         
    "texture creation failed",              // KECREATE_TEXTURE_FAIL           
    "texture blend mode set failed",        // KESET_TEXTURE_BLEND_MODE_FAIL   
    "seek failed",                          // KESEEK_FAIL                     
    "failed to fill array of image frame",  // KEAV_IMAGE_FILL_ARRAY_FAIL      
    "update video failed",                  // KEUPDATE_VIDEO_FAIL             
    "update texture failed",                // KEUPDATE_TEXTURE_FAIL           
    "audio data fill failed",               // KEAUDIO_FILL_FAIL               
    "open font file failed",                // KEOPEN_FONT_FAIL
    "SDL surface creation failed",          // KECREATE_SDL_SURFACE_FAIL
    "msger init failed",                    // KEMSGER_INIT_FAIL
    "ttf init failed",                      // KETTF_INIT_FAIL
    "render msg failed",                    // KERENDER_MSG_FAIL
    "no first frame",                       // KENO_FIRST_FRAME
    "log file open failed",                 // KELOG_FILE_OPEN_FAIL
    "undefined error code",                 // KEUNDEF17
    "undefined error code",                 // KEUNDEF16
    "undefined error code",                 // KEUNDEF15
    "undefined error code",                 // KEUNDEF14
    "undefined error code",                 // KEUNDEF13
    "undefined error code",                 // KEUNDEF12
    "undefined error code",                 // KEUNDEF11
    "undefined error code",                 // KEUNDEF10
    "undefined error code",                 // KEUNDEF9 
    "undefined error code",                 // KEUNDEF8 
    "undefined error code",                 // KEUNDEF7 
    "GUI window init failed",               // KEGUI_WINDOW_INIT_FAIL
    "GUI logger init failed",               // KEGUI_LOG_INIT_FAIL
    "GUI video widget init failed"          // KEGUI_VIDEO_WIDGET_INIT_FAIL
}; 

const char *                       kerr2str     (int err_code);
AVPLAYERWIDGET_EXPORT const char * getErrString (int err_code);


#endif /* _AVPLAYERWIDGET_ERROR_H_ */
