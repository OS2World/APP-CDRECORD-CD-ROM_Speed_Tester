/* CD drive functions (C) 1998-1999 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOSDEVIOCTL
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "miscsam.h"
#include "pmsam.h"
#include "readcd.h"

const char TAG[4] = {'C','D','0','1'};

CD_drive::~CD_drive()
{
   free(trackInfo);
   if(opened) close();
}

CD_drive::CD_drive()
{
   memset(this,0,sizeof(*this));
}


bool CD_drive::open(char *drive)
{
   ULONG ulAction;
   ULONG rc;

   rc = DosOpen(drive, &hCDDrive, &ulAction, 0,
                FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY |
                OPEN_FLAGS_DASD, NULL);

   if(rc)
   {
      updateError("DosOpen failed with return code %d opening drive %s",rc,drive);
      return false;
   }

   opened = true;
   return true;
}

bool CD_drive::close()
{
   ULONG rc;
   rc = DosClose(hCDDrive);
   opened = false;
   if(rc)
   {
      updateError("DosClose failed with return code %d closing drive",rc);
      return false;
   }

   return true;
}

bool CD_drive::readCDInfo()
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   char tag[4];

   memcpy(tag,TAG,4);

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIODISK,
                    tag, 4, &ulParamLen, &cdInfo,
                    sizeof(cdInfo), &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x reading CD info",rc);
   else
      returnBool = TRUE;

   return returnBool;
}

bool CD_drive::readTrackInfo(char track, CDTRACKINFO *trackInfo2)
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;

   CDAUDIOTRACKINFOPARAM trackParam;
   CDAUDIOTRACKINFODATA  trackInfo[2];

   memcpy(trackParam.signature,TAG,4);

   trackParam.trackNum = track;
   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIOTRACK,
                    &trackParam, sizeof(trackParam),
                    &ulParamLen, &trackInfo[0],
                    sizeof(trackInfo[0]), &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x reading track %d info",rc,track);
   else
   {
      trackParam.trackNum = track+1;
      rc = 0;
      if(trackParam.trackNum <= cdInfo.lastTrack)
      {
         rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_GETAUDIOTRACK,
                          &trackParam, sizeof(trackParam),
                          &ulParamLen, &trackInfo[1],
                          sizeof(trackInfo[1]), &ulDataLen);

         if(rc)
            updateError("DosDevIOCtl failed with return code 0x%x",rc);
      }
      else
          trackInfo[1].address = cdInfo.leadOutAddress;

      if(!rc)
      {
         ULONG LBA[2];
         MSF length;

         LBA[0] = getLBA(trackInfo[0].address);
         LBA[1] = getLBA(trackInfo[1].address);

         /* -150 because we want length, not an address */
         length = getMSF(LBA[1]-LBA[0]-150);

         trackInfo2->start = trackInfo[0].address;
         trackInfo2->end = trackInfo[1].address;
         trackInfo2->length = length;
         trackInfo2->size = (LBA[1]-LBA[0])*2352;
         trackInfo2->data = (trackInfo[0].info & 0x40) ? TRUE : FALSE;
         trackInfo2->channels = (trackInfo[0].info & 0x80) ? 4 : 2;
         trackInfo2->number = track;

         returnBool = TRUE;
      }
   }
   return returnBool;
}


bool CD_drive::fillTrackInfo()
{
   int i,e;

   trackInfo = (CDTRACKINFO *) realloc(trackInfo, getCount() * sizeof(*trackInfo));

   e = 0;
   for(i = cdInfo.firstTrack; i <= cdInfo.lastTrack; i++)
      if(!readTrackInfo(i, &trackInfo[e++]))
         return false;

   return true;
}


bool CD_drive::play(char track)
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   CDPLAYAUDIOPARAM playParam;
   CDTRACKINFO trackInfo;

   memcpy(playParam.signature,TAG,4);
   playParam.addressingMode = MODE_MSF;

   if(!readTrackInfo(track, &trackInfo))
       return FALSE;
   playParam.start = trackInfo.start;
   playParam.end = trackInfo.end;

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_PLAYAUDIO,
                    &playParam, sizeof(playParam),
                    &ulParamLen, NULL,
                    0, &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x playing track %d",rc,track);
   else
      returnBool = TRUE;

   return returnBool;
}

bool CD_drive::stop()
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   char tag[4];

   memcpy(tag,TAG,4);

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMAUDIO, CDROMAUDIO_STOPAUDIO,
                    tag, 4, &ulParamLen, NULL, 0, &ulDataLen);
   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x stopping playing.",rc);
   else
      returnBool = TRUE;
   return returnBool;
}

bool CD_drive::readSectors(CDREADLONGDATA data[], ULONG number, ULONG start)
{
   ULONG ulParamLen;
   ULONG ulDataLen;
   ULONG rc;
   BOOL returnBool = FALSE;
   CDREADLONGPARAM readParam = {0};

   memcpy(readParam.signature,TAG,4);
   readParam.addressingMode = MODE_LBA;
   readParam.numberSectors = number;
   readParam.startSector = start;

   rc = DosDevIOCtl(hCDDrive, IOCTL_CDROMDISK, CDROMDISK_READLONG,
                    &readParam, sizeof(readParam), &ulParamLen,
                    data, sizeof(*data)*number, &ulDataLen);

   if(rc)
      updateError("DosDevIOCtl failed with return code 0x%x reading sector %d-%d",rc,start,start+number-1);
   else
      returnBool = TRUE;
   return returnBool;
}
