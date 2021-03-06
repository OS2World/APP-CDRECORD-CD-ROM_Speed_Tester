typedef struct
{
	UCHAR frame;
	UCHAR second;
	UCHAR minute;
	UCHAR empty;
} MSF;

#define MODE_LBA 0
#define MODE_MSF 1

typedef struct
{
   UCHAR signature[4];
   UCHAR addressingMode; /* 0 = LBA, 1 = MSF */
   MSF start; /* can be LBA */
   MSF end;
} CDPLAYAUDIOPARAM;

typedef struct
{
	UCHAR firstTrack;
	UCHAR lastTrack;
	MSF leadOutAddress;
} CDAUDIODISKINFODATA;

typedef struct
{
	UCHAR signature[4];
	UCHAR trackNum;
} CDAUDIOTRACKINFOPARAM;

typedef struct
{
	MSF address;
	UCHAR info;
	#if 0
		bit 7  4 or 2 channels
		bit 6  data or audio track
		bit 5  copy ok or not ok
		bit 4  with or without preemp
		bit 3-0 ADR stuff?? whatever that is
	#endif
} CDAUDIOTRACKINFODATA;

typedef struct
{
   MSF start;
   MSF end;
   MSF length;
   ULONG size;
   BOOL data;
   USHORT channels;
   USHORT number;
} CDTRACKINFO;


#pragma pack(1)
typedef struct
{
   UCHAR signature[4];
   UCHAR addressingMode; /* 0 = LBA, 1 = MSF */
   USHORT numberSectors;
   ULONG startSector;
   UCHAR reserved;
   UCHAR interleaveSize;
   UCHAR interleaveSkipFactor;
} CDREADLONGPARAM;
#pragma pack()

typedef struct
{
   UCHAR sync[12];
   UCHAR header[4];
   UCHAR data[2048];
   UCHAR EDC_ECC[288];
} CDREADLONGDATA;


class CD_drive
{
   public:
      CD_drive();
      ~CD_drive();

      bool open(char *drive);
      bool close();
      bool readCDInfo();
      bool fillTrackInfo();
      bool play(char track);
      bool stop();
      bool readSectors(CDREADLONGDATA data[], ULONG number, ULONG start);

      char getCount() { return cdInfo.lastTrack - cdInfo.firstTrack + 1; };
      CDTRACKINFO *getTrackInfo(char pos)
         { return &trackInfo[pos]; };
      CDAUDIODISKINFODATA *getCDInfo()
         { return &cdInfo; };

      USHORT getTime()
         { return (cdInfo.leadOutAddress.minute*60 + cdInfo.leadOutAddress.second) -
                  (trackInfo[0].start.minute*60 + trackInfo[0].start.second); };


      static ULONG getLBA(MSF input)
      {
         return (input.minute * 4500 + input.second * 75 + input.frame - 150);
      };

      static MSF getMSF(ULONG input)
      {
         MSF output;

         input += 150;
         output.frame = input % 75;
         input /= 75;
         output.second = input % 60;
         output.minute = input / 60;

         return output;
      };


   protected:
      bool opened; /* hCDDrive cannot be negative and 0 is a valid handle
                      so I keep another variable around */
      HFILE hCDDrive;
      CDAUDIODISKINFODATA cdInfo;
      CDTRACKINFO *trackInfo;


      bool readTrackInfo(char track, CDTRACKINFO *trackInfo);
};

