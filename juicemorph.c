/*

   JuiceMorph v1.0

   (C) Copyright 2014 Andre Sklenar (andre.sklenar@gmail.com)

   This program is free software; you can redistribute it and/or modify
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   Author's web: www.juicelab.cz
   This work's api calls and other ladspa boilerplate
   is derived from the WubFlip http://www.alexs.org/ladspa/ by Alex Sisson (alexsisson@gmail.com)
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ladspa.h"
#include <time.h>
#include <math.h>

struct juicemorph_t {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *ctl1;
	LADSPA_Data *ctl2;
	LADSPA_Data *ctl3;
};

LADSPA_Data buffer[50000*10];
long bufferRecPos;
long bufferPlayPos;
long bufferLength;

LADSPA_Handle juicemorph_instantiate(const LADSPA_Descriptor *D, unsigned long sr) {
	return malloc(sizeof(struct juicemorph_t));
}

void juicemorph_activate(LADSPA_Handle handle) {
}

void juicemorph_connectport(LADSPA_Handle handle, unsigned long port, LADSPA_Data *data) {
	struct juicemorph_t *w = (struct juicemorph_t*)handle;
	switch (port) {
		case 0: w->in         = data; break;
		case 1: w->out        = data; break;
		case 2: w->ctl1      = data; break;
		case 3: w->ctl2      = data; break;
		case 4: w->ctl3		 = data; break;
	}
}


//that's just creating noise, kinda useless, not in use
void randomMove(LADSPA_Data ctl) {
	unsigned long i;
	for (i=0; i<bufferLength; i++) {
		float a = ctl;
		buffer[i] += ((float)rand()/(float)(RAND_MAX)) * a;
		buffer[i] -= ((float)rand()/(float)(RAND_MAX)) * a;
		if (buffer[i]>1) buffer[i]=-1;
		if (buffer[i]<-1) buffer[i]=1;
	}
}

//wrap around buffer length for swapping outside the buffer range
long wrap(long number) {
	if (number>bufferLength) {
		return (number-bufferLength);
	} else {
		return number;
	}
}

void randomSwap(LADSPA_Data ctl1, LADSPA_Data ctl2) {
	unsigned long i;
	unsigned long chunkSize = (rand() % (int)round(ctl2));
	chunkSize += 1000;
	for (i = 0; i<bufferLength; i+=chunkSize) {
		
		//swap chunks
		float dice = ((float)rand()/(float)(RAND_MAX));
		float dice2 = ((float)rand()/(float)(RAND_MAX));
		if (dice>ctl1) {
			//swap chunks
			chunkSize = (rand() % (int)round(ctl2));
			chunkSize += 1000;
			LADSPA_Data buf = 0;
			unsigned long j;
			//swap them
			for (j=1; j<chunkSize; j++) {
					buf = buffer[wrap(i+j)];
					buffer[wrap(i+j)] = buffer[wrap(i+j+chunkSize)];
					buffer[wrap(i+j+chunkSize)] = buf;
			}
			//reverse the chunk?
			if (dice2>ctl1) {
				LADSPA_Data tempBuf[chunkSize];
				for (j=1; j<chunkSize; j++) {
					//fill a temp buffer
					tempBuf[j] = buffer[wrap(i+j)]; 
				}
				for (j=1; j<chunkSize; j++) {
					//fill a temp buffer
					buffer[wrap(i+j)] = tempBuf[chunkSize-j];
				}
			}
			//cross-fade for 1000 samples
			for (j=1; j<1000; j++) {
				buf = buffer[wrap(i+j)];
				buffer[wrap(i+j)] *= j/1000;
				buffer[wrap(i+j)] += buffer[wrap(i+j+chunkSize)]/j;

				buffer[wrap(i+j+chunkSize)]*= j/1000;
				buffer[wrap(i+j+chunkSize)]+= buf/j;
			}
		}
	}
	
}


/* process */
void juicemorph_run(LADSPA_Handle handle, unsigned long n) {
	unsigned long i;
	
	struct juicemorph_t *w = (struct juicemorph_t*)handle;
	LADSPA_Data ctl1 = *(w->ctl1);
	LADSPA_Data ctl2 = *(w->ctl2);
	LADSPA_Data ctl3 = *(w->ctl3);

	for(i=0;i<n;i++) {
		if (ctl3>0 && bufferRecPos<(50000*10)) {
			//record mode
			buffer[bufferRecPos] = w->in[i];
			bufferRecPos++;
			w->out[i] = w->in[i];
		} else {
			if (bufferRecPos!=0) {
				bufferLength = bufferRecPos;
				bufferRecPos = 0;
				bufferPlayPos = 0;
			}

			//playback time
			w->out[i] = buffer[bufferPlayPos];
			bufferPlayPos++;
			if (bufferPlayPos>=bufferLength) {
				//reprocess
				randomSwap(ctl1, ctl2);

				//go loop
				bufferPlayPos = 0;
			}
		}
	}
}

void juicemorph_cleanup(LADSPA_Handle handle) {
	free(handle);
}

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	char **pn;
	LADSPA_PortDescriptor *pd;
	LADSPA_PortRangeHint *prh;

	if(index!=0)
		return NULL;

	static LADSPA_Descriptor LD;
	static int needsInit = 1;

	if(needsInit == 0)
		return &LD;

	LD.UniqueID   = 4104;
	LD.Label      = strdup("JuiceMorph");
	LD.Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	LD.Name       = strdup("JuiceMorph");
	LD.Maker      = strdup("Andre Sklenar (andre.sklenar@gmail.com)");
	LD.Copyright  = strdup("(C) Andre Sklenar 2014");
	LD.PortCount  = 5;

	pd = malloc(sizeof(LADSPA_PortDescriptor)*LD.PortCount);
	pd[0] = LADSPA_PORT_AUDIO   | LADSPA_PORT_INPUT;
	pd[1] = LADSPA_PORT_AUDIO   | LADSPA_PORT_OUTPUT;
	pd[2] = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
	pd[3] = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
	pd[4] = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
	LD.PortDescriptors = pd;

	pn = malloc(sizeof(char*)*LD.PortCount);
	pn[0] = strdup("Input");
	pn[1] = strdup("Output");
	pn[2] = strdup("Unpropability");
	pn[3] = strdup("Chunk Size");
	pn[4] = strdup("Record");
	LD.PortNames = (const char **)pn;

	prh = malloc(sizeof(LADSPA_PortRangeHint)*LD.PortCount);
	prh[0].HintDescriptor  = 0;
	prh[1].HintDescriptor  = 0;
	prh[2].HintDescriptor  = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
	prh[2].LowerBound      = 0;
	prh[2].UpperBound      = 1;
	prh[3].HintDescriptor  = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
	prh[3].LowerBound      = 1000;
	prh[3].UpperBound      = 100000;
	prh[4].HintDescriptor  = LADSPA_HINT_TOGGLED;
	prh[4].LowerBound      = 0;
	prh[4].UpperBound      = 1;
	LD.PortRangeHints      = prh;

	LD.instantiate         = juicemorph_instantiate;
	LD.activate            = juicemorph_activate;
	LD.connect_port        = juicemorph_connectport;
	LD.run                 = juicemorph_run;
	LD.cleanup             = juicemorph_cleanup;
	LD.run_adding          = NULL;
	LD.set_run_adding_gain = NULL;
	LD.deactivate          = NULL;

	// this was on init()
	// it should be placed in the juicemorph_t struct
	bufferRecPos = 0;
	bufferPlayPos = 0;
	bufferLength = 0;
	srand((unsigned int)time(NULL));

	return &LD;
}
