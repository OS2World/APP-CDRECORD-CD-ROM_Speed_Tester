#
#  testspeed.exe makefile for NMAKE and VAC++
#

CC = icc
AS = wasm
RC = rc

.ASM.OBJ:
   $(AS) $(AFLAGS) $*.ASM

.RC.RES:
   $(RC) -r $*.rc

all: testspeed.exe

CFLAGS = /Tdp /Q /B"/ST:128000"

!IFDEF DEBUG
CFLAGS = /Ti $(CFLAGS)
!ENDIF

!IFNDEF NOTOPT
CFLAGS = /B "/PACKCODE /PACKDATA" /Gf /Gi /O /Ol /G5 $(CFLAGS)
!ENDIF

OBJECTS = testspeed.obj readcd.obj pmsam.obj prfsam.obj

RESOURCE =
LIBS =

testspeed.exe: $(OBJECTS)
   $(CC) $(CFLAGS) /Fm$* /Fe$@ $(OBJECTS) $(LIBS)

prfsam.obj:
pmsam.obj: prfsam.h pmsam.h
readcd.obj: readcd.h pmsam.h miscsam.h
testspeed.obj: miscsam.h readcd.h
