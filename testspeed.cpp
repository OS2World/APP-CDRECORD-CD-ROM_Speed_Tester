/* CD-ROM speed tester (C) 1998-1999 Samuel Audet <guardia@cam.org> */

#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <float.h>

#include "miscsam.h"
#include "readcd.h"

#define CHUNK 27

void displayHelp(char *program)
{
    printf("CD-ROM Speed Tester 1.0  (C) 1998 Samuel Audet <guardia@cam.org>\n"
           "\n"
           "Syntax:\n"
           "   %s <cd drive letter> [/c <bogus cache size in kB>] [/t <track,track,...>] [/s <fake cd writing speed>]\n"
           "\n"
           "The default bogus cache size is 4096 kB.  "
           "By default, all tracks are tested.\n"
           "The default fake CD writing speed is 4x.\n",program);
}

int main(int argc, char *argv[])
{
   CD_drive drive;
   ULONG firstsector, lastsector, sector;
   CDREADLONGDATA data[CHUNK];
   double *timeTable;
   double lowestSpeed;
   int sizeTable;
   int farpos, curpos, i, e;
   char *driveLetter = NULL;
   short cacheSize = 4096, cacheStatus;
   double cdSpeed = 4.0;
   double lastTime, curTime, timer;
   char tracksToTest[256];
   int hitBottom; /* how many times did the cache empty */

   /* all tracks */
   for(i = 0; i < 256; i++)
      tracksToTest[i] = i+1;

   if(argc == 1)
   {
      displayHelp(argv[0]);
      return 0;
   }


   for(i = 1; i < argc; i++)
   {
      if(argv[i][0] == '/' || argv[i][0] == '-')
      {
         switch(argv[i][1])
         {
            case 'c': case 'C':
               cacheSize = atoi(argv[++i]); break;
            case 't': case 'T':
            {
               char *comma = argv[++i];
               e = 0;
               do 
               {
                  tracksToTest[e++] = atoi(comma);
               } while((comma = strchr(comma,',')+1) != (char*)1);
               tracksToTest[e] = 0;
               break;
            }
            case 's': case 'S':
               cdSpeed = atoi(argv[++i]); break;
            case '?':
            default:
               displayHelp(argv[0]); return 0;
         }
      }
      else
         driveLetter = argv[i];
   }

   if(!driveLetter)
   {
      fprintf(stderr,"Error: no drive letter specified.");
      return 1;
   }

   printf("Faking a %d kB cache writing at %#.3gx.\n",cacheSize,cdSpeed);

   sizeTable = cacheSize*1024/2352/CHUNK;
   timeTable = (double*)alloca(sizeTable*sizeof(timeTable[0]));

   drive.open(driveLetter);
   drive.readCDInfo();
   drive.fillTrackInfo();

   for(i = 0; i < 256 && tracksToTest[i]; i++)
   {
      bool found = false;

      /* finding the track */
      for(e = 0; e < drive.getCount() && !found; e++)
         if(drive.getTrackInfo(e)->number == tracksToTest[i])
            found = true;
      if(!found)
         continue;
      e--;

      timeTable[0] = (double) clock()/CLOCKS_PER_SEC;
      firstsector = sector = CD_drive::getLBA(drive.getTrackInfo(e)->start);
      lastsector = CD_drive::getLBA(drive.getTrackInfo(e)->end);
      curpos = farpos = 0;
      lowestSpeed = _INF;
      cacheStatus = 0;
      lastTime = (double) clock()/CLOCKS_PER_SEC;
      hitBottom = 0;

      /* spin up the drive */
      drive.readSectors(data,1,sector);
      DosSleep(2000);

      printf("Testing track %d...\n",drive.getTrackInfo(e)->number);

      while(sector < lastsector-1)
      {
         int timeLength; /* length between two times */
         double speed, xspeed;
         int readLength = min(CHUNK,lastsector-sector-1); /* amount of sectors to read */
         BOOL updateScreen = FALSE;

         curTime = (double) clock()/CLOCKS_PER_SEC;
         timer += (curTime - lastTime);
         if(timer > 0.5 || sector+readLength >= lastsector-1)
            updateScreen = TRUE;

         if(updateScreen)
         {
            if(_kbhit() && _getch() == 27)
            {
               i = drive.getCount();
               break;
            }

            fflush(stdin);

            printf("Reading %6.1d -> %d ",sector, lastsector-1);
         }
         drive.readSectors(data,readLength,sector);
         sector += readLength;

         curpos++;
         if(curpos >= sizeTable)
            curpos = 0;

         if(curpos == farpos)
         {
            farpos++;
            if(farpos >= sizeTable)
               farpos = 0;
         }


         if(curpos > farpos)
            timeLength = (curpos-farpos)*CHUNK*2352;
         else
            timeLength = (sizeTable-farpos+curpos)*CHUNK*2352;

         timeTable[curpos] = (double) clock()/CLOCKS_PER_SEC;

         speed = timeLength/(timeTable[curpos]-timeTable[farpos])/1024;
         xspeed = speed /150;

         if(updateScreen)
            printf(" Avg Speed: %#8.6g kB/s == %#.3gx ", speed, xspeed);


         cacheStatus += readLength * 2352/1024;
         if(sector-firstsector > sizeTable*CHUNK)
            cacheStatus -= (curTime-lastTime) * cdSpeed * 150;
         /* truncate the cache data if it's too big */
         if(cacheStatus > cacheSize)
            cacheStatus = cacheSize;
         else if(cacheStatus <= 0)
         {
            cacheStatus = 0;
            hitBottom++;
         }
         if(updateScreen)
            printf(" Cache: %5.1d kB  \r",cacheStatus);


         if(speed < lowestSpeed && sector-firstsector > sizeTable*CHUNK)
            lowestSpeed = speed;

         if(updateScreen)
         {
            fflush(stdout);
            timer = 0.0;
         }
         lastTime = curTime;
      }
      printf("\nLowest Speed: %#8.6g kB/s == %#.3gx  Hit Bottom: %d times\n",lowestSpeed,lowestSpeed/150, hitBottom);
   }

   drive.close();

   return 0;
}
