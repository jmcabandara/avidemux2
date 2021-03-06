/***************************************************************************
                          gui_navigate.cpp  -  description
                             -------------------

            GUI Part of get next frame, previous key frame, any frame etc...

    
    copyright            : (C) 2002/2008 by mean
    email                : fixounet@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "ADM_cpp.h"

#include "ADM_default.h"
#include "avi_vars.h"

#include <math.h>
#include "prototype.h"
#include "gui_action.hxx"
#include "gtkgui.h"

#include "DIA_coreToolkit.h"
#include "ADM_commonUI/DIA_busy.h"
#include "ADM_commonUI/GUI_ui.h"
#include "DIA_enter.h"
#include "DIA_coreToolkit.h"
#include "ADM_vidMisc.h"
#include "ADM_preview.h"

static ADMCountdown  NaggingCountDown(5000); // Wait 5 sec before nagging again for cannot seek
static void A_timedError(bool force, const char *s);

extern uint8_t DIA_gotoTime(uint32_t *hh, uint32_t *mm, uint32_t *ss,uint32_t *ms);
bool   GUI_infiniteForward(uint64_t pts);
bool   GUI_lastFrameBeforePts(uint64_t pts);
bool   GUI_SeekByTime(int64_t time);
bool A_jumpToTime(uint32_t hh,uint32_t mm,uint32_t ss,uint32_t ms);
/**
    \fn HandleAction_Navigate

*/
void HandleAction_Navigate(Action action)
{
static int ignore_change=0;
    switch (action)
      {
    
      case ACT_Scale:
        if (!ignore_change)
        {
          uint32_t nf;
          ignore_change++;
          nf = GUI_GetScale ();
          ADM_info("Scale :%" PRIu32"\n",nf);
          double tme=nf;
          double totalDuration=(double) video_body->getVideoDuration();
          tme*=totalDuration;
          tme/=ADM_SCALE_SIZE;
          uint64_t pts=5000+(uint64_t)tme; // aim a bit higher to avoid double "search previous" events
          if(pts>totalDuration) pts=totalDuration;
          
          ADM_info("Scale Time:%" PRIu64" ms (total=%" PRIu64" ms)\n",pts/1000,video_body->getVideoDuration()/1000);
          ADM_info("Scale Time:%s ms \n",ADM_us2plain(pts));
           if(false==video_body->getPKFramePTS(&pts))
            {
                if(false==video_body->getNKFramePTS(&pts))
                {
                    ADM_warning("Cannot seek to %" PRIu64" ms\n",pts/1000);
                    ignore_change--;
                    break;
                }
            }
             ADM_info("Seeking to  Time:%s ms \n",ADM_us2plain(pts));
            if(true!=admPreview::seekToIntraPts(pts))
            {
                ADM_warning("Scale: Seeking to intra at %" PRIu64" ms failed\n",pts/1000);
            }
            UI_setCurrentTime(pts);
            UI_purge();
            ignore_change--;
        }
        break;
      case ACT_GotoMarkA:
      case ACT_GotoMarkB:
            {
                uint64_t pts;
                if(action==ACT_GotoMarkA) 
                        pts=video_body->getMarkerAPts();
                else  
                        pts=video_body->getMarkerBPts();
                if(false==video_body->goToTimeVideo(pts))
                {
                    if(action==ACT_GotoMarkA)
                    {
                        ADM_warning("Go to Marker A: Seek to time %s ms failed\n",ADM_us2plain(pts));
                    }else // PTS returned by getMarkerBPts() may be beyond the last frame.
                          // Go to the last frame then.
                    {
                        pts=video_body->getLastKeyFramePts();
                        if(pts==ADM_NO_PTS) 
                                break;     
                        GUI_infiniteForward(pts);
                    }
                }
                admPreview::samePicture();
                GUI_setCurrentFrameAndTime();
            }
            break;

      case ACT_PreviousKFrame:
        GUI_PreviousKeyFrame();
        break;
      case ACT_PreviousFrame:
        GUI_PrevFrame();
        break;
    case ACT_Forward1Mn:
          GUI_SeekByTime(60*1000LL*1000LL);
          break;
        case ACT_Back1Mn:
          GUI_SeekByTime(-60*1000LL*1000LL);
          break;        
      case ACT_Forward4Seconds:
        GUI_SeekByTime(4000000LL);
        break;

      case ACT_Forward2Seconds:
        GUI_SeekByTime(2000000LL);
    break;

      case ACT_Forward1Second:
        GUI_SeekByTime(1000000LL);
        break;

      case ACT_Back4Seconds:
        GUI_SeekByTime(-4000000LL);
        break;

      case ACT_Back2Seconds:
        GUI_SeekByTime(-2000000LL);
        break;

      case ACT_Back1Second:
        GUI_SeekByTime(-1000000LL);
    break;

      case ACT_NextFrame:
        GUI_NextFrame();
      break;
      case ACT_NextKFrame:
        GUI_NextKeyFrame();
      break;
      case ACT_NextBlackFrame:
        GUI_NextBlackFrame();
      break;
      case ACT_PrevBlackFrame:
        GUI_PrevBlackFrame();
      break;
      case ACT_End:
        {
            uint64_t pts=video_body->getLastKeyFramePts();
            if(pts==ADM_NO_PTS) break;    
            GUI_infiniteForward(pts);
            admPreview::samePicture();
            GUI_setCurrentFrameAndTime();
        }
            break;
      case ACT_Begin:
            video_body->rewind();
            admPreview::samePicture(); // Ugly FIXME TODO
             GUI_setCurrentFrameAndTime();
            //GUI_GoToKFrameTime(0);
            break;
      case ACT_GotoTime:
      {
           // Get current time
            uint64_t pts=admPreview::getCurrentPts();

          uint32_t mm, hh, ss, ms;
            ms2time((uint32_t)(pts/1000),&hh,&mm,&ss,&ms);
          if (DIA_gotoTime(&hh, &mm, &ss,&ms))
          {
            A_jumpToTime(hh, mm, ss, ms);
          }
      }
      break;
      default:
      ADM_assert(0);
      break;
      }
}
/**
    \fn GUI_NextFrame
    \brief next frame
*/
bool GUI_NextFrame(void)
{
    if (playing)
        return false;
    if (!avifileinfo)
        return false;

    if(!admPreview::nextPicture())
        return false;
    GUI_setCurrentFrameAndTime();
    UI_purge();
    return true;
}


/**
    \fn GUI_NextKeyFrame
    \brief Go to the next keyframe
*/
bool GUI_NextKeyFrame(void)
{
    static bool firstError = true;

    if (playing)
        return false;
    if (!avifileinfo)
        return false;

    if (!admPreview::nextKeyFrame())
      {
        bool force = firstError;
        firstError = false;
        A_timedError(force, QT_TRANSLATE_NOOP("navigate","Cannot go to next keyframe"));
        return false;
      }
    GUI_setCurrentFrameAndTime();
    UI_purge();
    return true;
}

/**
    \fn GUI_GoToKFrameTime
    \brief Go to keyframe at given exact time
*/
bool GUI_GoToKFrameTime(uint64_t exactTime)
{
    if (playing)
        return false;
    if (!avifileinfo)
        return false;

    if(!admPreview::seekToIntraPts(exactTime))
        return false;
    //admPreview::samePicture(); // why a second time?
    GUI_setCurrentFrameAndTime();
    UI_purge();
    return true;
}
/**
    \fn GUI_GoToFrame
    \brief go to a given frame. Half broken, do not use.
*/
bool GUI_GoToFrame(uint32_t frame)
{
#if 0
    uint32_t flags;

    if (playing)
        return 0;
    if (!avifileinfo)
        return 0;
    if (frame >= avifileinfo->nb_frames)
        return 0;

    if (!video_body->setCurrentFrame(frame))
        return 0;    

    if(!admPreview::samePicture()) return 0;
    GUI_setAllFrameAndTime();
#endif
    return 1;
}

/**
    \fn GUI_PreviousKeyFrame
    \brief Go to previous keyframe
*/
bool GUI_PreviousKeyFrame(void)
{
    static bool firstError = true;

    if (playing)
        return false;
    if (!avifileinfo)
        return false;

    if (!admPreview::previousKeyFrame())
      {
        bool force = firstError;
        firstError = false;
        A_timedError(force, QT_TRANSLATE_NOOP("navigate","Cannot go to previous keyframe"));
        return false;
      }
    GUI_setCurrentFrameAndTime();
    UI_purge();
    return true;
}

uint8_t A_rebuildKeyFrame(void)
{
//    return video_body->rebuildFrameType();
    return 1;
}
/**
    \fn GUI_PrevFrame
    \brief Go to current frame -1
*/
bool GUI_PrevFrame(void)
{
    if (playing)
        return false;
    if (!avifileinfo)
        return false;

    if (!admPreview::previousPicture())
      {
//        We're probably at the beginning of the file ...
//            GUI_Error_HIG(QT_TRANSLATE_NOOP("navigate","Error"),    QT_TRANSLATE_NOOP("navigate","Cannot go to previous frame"));
        return false;
      }
    GUI_setCurrentFrameAndTime();
    UI_purge();
    return true;
}
/**
      \fn A_jogRead
      \brief read an average value of jog
*/
#define NB_JOG_READ           3
#define JOG_READ_PERIOD_US    5*1000    // 5ms
#define JOG_THRESH1           40
#define JOG_THRESH2           80
#define JOG_THRESH1_PERIOD    100*1000    // us
#define JOG_THRESH2_PERIOD    40*1000
#define JOG_THRESH3_PERIOD    500
/**
    \fn A_jogRead
    \brief Read an average value of jog
*/
uint32_t A_jogRead(void)
{
    int32_t sum = 0, v;
    for (int i = 0; i < NB_JOG_READ; i++)
      {
      v = UI_readJog();
      if (abs(v) < 10)
          v = 0;
      sum += v;
      ADM_usleep(JOG_READ_PERIOD_US);
      }
    return sum / NB_JOG_READ;
}
#define REFRESH 10000
/**
      \fn     A_jog
      \brief  Handle jogshuttle widget
*/
void A_jog(void)
{
    int32_t r;
    uint32_t a;
    uint32_t slip;
    static int jog = 0;
    if (jog)
    return;
    jog++;
    while ((r = A_jogRead()))
      {
      a = abs(r);
      printf("%d \n", r);
      if (a < JOG_THRESH1)
          slip = JOG_THRESH1_PERIOD;
      else if (a < JOG_THRESH2)
          slip = JOG_THRESH2_PERIOD;
      else
          slip = JOG_THRESH3_PERIOD;

      if (r > 0)
          GUI_NextKeyFrame();
      else
          GUI_PreviousKeyFrame();
      UI_purge();
      for (int i = 0; i < slip / REFRESH; i++)
        {
        UI_purge();
        ADM_usleep(REFRESH);
        UI_purge();
        }
      }
    jog--;
}
/**
    \fn     GUI_setAllFrameAndTime
    \brief  Update all  informations : current frame # and current time, total frame ...

*/
void GUI_setAllFrameAndTime(void)
{
    UI_setTotalTime(video_body->getVideoDuration());
    // progress bar
    GUI_setCurrentFrameAndTime(0);
}

/**
    \fn GUI_setCurrentFrameAndTime
    \brief Update some informations : current frame # and current time
*/
void GUI_setCurrentFrameAndTime(uint64_t offset)
{
    uint64_t pts=admPreview::getCurrentPts()+offset;
    double len;
   
    UI_setCurrentTime(pts);
    len=pts;
    len*=ADM_SCALE_SIZE;
    len/=video_body->getVideoDuration();  
    GUI_SetScale(len);
}

/**
    \fn A_jumpToTime
    \brief Jump to a given time
*/
bool A_jumpToTime(uint32_t hh,uint32_t mm,uint32_t ss,uint32_t ms)
{
    uint64_t pts;
    pts=hh*3600+mm*60+ss;
    pts*=1000;
    pts+=ms;
    pts*=1000;
    if(pts > video_body->getVideoDuration())
    {
        ADM_warning("Cannot navigate beyond the end of the video\n");
        return false;
    }
    uint64_t lastpts=pts;
    if(false==video_body->getNKFramePTS(&lastpts)) // at the end of the video, be careful
    {
        if(false==video_body->getPKFramePTS(&lastpts))
            return false;
        GUI_infiniteForward(lastpts);
        lastpts=admPreview::getCurrentPts();
        if(pts>=lastpts) // if at or beyond the last frame but within the total duration
            return GUI_GoToTime(lastpts); // go to the last frame
    }
    if(false==GUI_lastFrameBeforePts(pts)) // we are probably at the beginning of the video,
        video_body->rewind(); // go to the first frame then
    admPreview::samePicture();
    GUI_setCurrentFrameAndTime();
    return true;
}
/**
    \fn GUI_SeekByTime
*/
bool GUI_SeekByTime(int64_t time)
{
    uint64_t pts=admPreview::getCurrentPts();

    if (time < 0 && pts < -time)
    {   // we can't assume that pts=0 were legitimate, rewind to the first frame instead
        video_body->rewind();
        admPreview::samePicture();
        GUI_setCurrentFrameAndTime();
        return true;
    }else
    {
        pts += time;
    }
    ADM_info("Seek to:%s ms \n",ADM_us2plain(pts));
    return GUI_lastFrameBeforePts(pts);
}

/**
    \fn GUI_GoToTime
*/
bool GUI_GoToTime(uint64_t time)
{

    // We have to call the editor as the frames needed to decode the target frame may be hidden
    if(false==video_body->goToTimeVideo(time))
    {
        GUI_Error_HIG(QT_TRANSLATE_NOOP("navigate","Seek"), QT_TRANSLATE_NOOP("navigate","Error seeking to %" PRIu64" ms"),time/1000);
    }
    admPreview::samePicture();
    GUI_setCurrentFrameAndTime();
    return true;
}   
/**
 * \brief go forward as much as possible from pts
 * @param pts
 * @return 
 */
bool GUI_infiniteForward(uint64_t pts)
{
    admPreview::deferDisplay(1);
    if(false==video_body->goToTimeVideo(pts))
    {
        ADM_warning("Seek to %s failed, retrying from the previous keyframe\n",ADM_us2plain(pts));
        // work around a possible inability to decode keyframe at pts
        uint64_t tmp=pts;
        if(!video_body->getPKFramePTS(&tmp))
            return false;
        ADM_info("Retrying from keyframe at %s\n",ADM_us2plain(tmp));
        if(false==video_body->goToTimeVideo(tmp))
        {
            ADM_error("Seek to the penultimate keyframe failed as well, giving up\n",ADM_us2plain(tmp));
            return false;
        }
    }
    while(admPreview::nextPicture())
    {
    }
    admPreview::deferDisplay(0);
    return true;
}

/**
    \fn GUI_lastFrameBeforePts
 */
bool GUI_lastFrameBeforePts(uint64_t pts)
{
    uint64_t tmp=pts;
    uint64_t current=admPreview::getCurrentPts();
    // Try to find a keyframe just before pts...
    if(video_body->getPKFramePTS(&tmp))
    {
        bool r=true;
        if(tmp<=current && pts>current) // within the same GOP, seeking forward
            tmp=current; // no need to go back to the keyframe
        else
            r=video_body->goToTimeVideo(tmp);
        // Starting from tmp, approach the last frame before pts
        if(r)
        {
            admPreview::deferDisplay(1);
            uint64_t inc=video_body->getFrameIncrement();
            while(tmp+inc <= pts) // otherwise we may overshoot
            {
                admPreview::nextPicture();
                tmp=admPreview::getCurrentPts();
            }
            admPreview::deferDisplay(0);
            admPreview::samePicture();
            GUI_setCurrentFrameAndTime();
            return true;
        }
    }
    return false;
}

/**
 * \fn A_timedError
 * \brief display error unless the last error is too recent
 * @param s
 */
void A_timedError(bool force, const char *s)
{
    if(force || NaggingCountDown.done()) // else still running, do not nag
    {
        NaggingCountDown.reset();
        GUI_Error_HIG(QT_TRANSLATE_NOOP("navigate","Error"),s);
        return;
    }
    NaggingCountDown.reset();
}

// EOF
