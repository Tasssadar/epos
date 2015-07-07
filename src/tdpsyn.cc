/*
 * 	epos/src/tdpsyn.cc
 * 	(c) 2000-2002 Petr Horak, horak@petr.cz
 * 	(c) 2001-2002 Jirka Hanika, geo@cuni.cz
 *
 *	tdpsyn version 2.5.0 (20.9.2002)
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *	
 *
 */

#include "epos.h"
#include "tdpsyn.h"
#include <math.h>
#include "endian_utils.h"

#define MAX_STRETCH	30	/* stretching beyond 30 samples per OLA frame must be done through repeating frames */
#define MAX_OLA_FRAME	4096	/* sanity check only */
#define HAMMING_PRECISION 15	/* hamming coefficient precision in bits */
#define LP_F0_STEP 8			/* step of F0 analysis for linear prediction */
#define LP_DECIM 10				/* F0 analysis decimation coeff. for linear prediction */
#define LP_F0_ORD 4				/* order of F0 contour LP analysis */
#define F0_FILT_ORD 9			/* F0 contour filter order */
#define LP_EXC_MUL 1.0			/* LP excitation multipicator */

#ifdef WIN32
#define snprintf _snprintf
#endif

void tdioven(char *p, int l);

/* F0 contour filter coefficients */
const double a[9] = {1,-6.46921563821389,18.43727805607084,-30.21344177474595,31.11962012720199,
					-20.62061607537661,8.58111044795433,-2.04983423923570,0.21516477151414};

const double b[9] = {0.01477848982115,-0.08930749676388,0.25181616063735,-0.43840842338467,0.52230821454923,
					-0.43840842338467,0.25181616063735,-0.08930749676388,0.01477848982115};

/* lp f0 contour filter coefficients (mean of 144 sentences from speaker Machac) */
// const double lp[LP_F0_ORD] = {-1.23761, 0.60009, -0.32046, 0.10699};

// these coeffs are from the version where the f0 is constant inside a syllable
const double lp[LP_F0_ORD] = {-0.900693, 0.043125, -0.003700, 0.069916};
// FIXME! lp coefficients must be configurable

/* Hamming coefficients for TD-PSOLA algorithm */
int hamkoe(int winlen, uint16_t *data, int e, int e_base)
{
	int i;
	double fn;
	fn = 2 * pii / (winlen - 1);
	for (i=0; i < winlen; i++) {
		data[i] = (uint16_t)((0.53999 - 0.46 * cos(fn * i)) * e / e_base * (1 << HAMMING_PRECISION));
	}
	return 0;
}


#if 0

int hamkoe(int winlen, double *data)
{
	D_PRINT(0, "%d\n", winlen);
	int i;
	double fn;
	fn = 2 * pii / (winlen - 1);
	for (i=0; i < winlen; i++)
		data[i] = 0.54 - 0.46 * cos(fn * i);
	return 0;
}

int median(int prev, int curr, int next, int ibonus)
{
	int lm;
	if ((prev <= curr) == (curr <= next)) lm = curr;
	else if ((prev <= curr) == (next <= prev)) lm = prev;
	else lm = next;
	return (abs(curr - lm) > ibonus) ? lm : curr;
}

#endif


/* Inventory file header structure */
struct tdi_hdr
{
	int32_t  magic;
	int32_t  samp_rate;
	int32_t  samp_size;
	int32_t  bufpos;
	int32_t  n_segs;
	int32_t  diph_offs;
	int32_t  diph_len;
	int32_t  res1;
	int32_t  res2;
	int32_t  ppulses;
	int32_t  res3;
	int32_t  res4;
};

tdpsyn::tdpsyn(voice *v)
{
	tdi_hdr *hdr;

	difpos = 0;

	tdi = claim(v->models, v->location, scfg->inv_base_dir, "rb", "inventory", tdioven);
	hdr = (tdi_hdr *)tdi->data;
	D_PRINT(0, "Got %d and config says %d\n", hdr->n_segs, v->n_segs);
	if (v->n_segs != hdr->n_segs) shriek(463, "inconsistent n_segs");
	if (sizeof(SAMPLE) != hdr->samp_size) shriek(463, "inconsistent samp_size");
	tdp_buff = (SAMPLE *)(hdr + 1);
	diph_offs = (int *)((char *)tdp_buff + sizeof(SAMPLE) * hdr->bufpos);
	diph_len = diph_offs + v->n_segs;
	ppulses = diph_len + v->n_segs;

	// this is debugging only!
	D_PRINT(0, "Samples are %d bytes long.\n", sizeof(SAMPLE) * hdr->bufpos);

	/* allocate the maximum necessary space for Hamming windows: */	
//	max_frame = 0;
//	for (int k = 0; k < v->n_segs; k++) {
//		int avpitch = average_pitch(diph_offs[k], diph_len[k]);
//		int maxwin = avpitch + MAX_STRETCH; //(int)(w->inv_samping_rate / 500);
//		if (max_frame < maxwin) max_frame = maxwin;
//	}
//	max_frame++;
//	if (max_frame >= MAX_OLA_FRAME || max_frame == 0) shriek(463, "Inconsistent OLA frame buffer size");
	//FIXME! max_frame=maxwin * (max_L - min_ori_L)

	max_frame = MAX_OLA_FRAME;
	
	wwin = (uint16_t *)xmalloc(sizeof(uint16_t) * (max_frame * 2));
	memset(wwin, 0, (max_frame * 2) * sizeof(*wwin));

	out_buff = (SAMPLE *)xmalloc(sizeof(SAMPLE) * max_frame * 2);
	memset(out_buff, 0, max_frame * 2 * sizeof(*out_buff));

	/* initialisation of lp prosody engine */
	if (v->lpcprosody) {
		int i;
		for (i = 0; i < LPC_PROS_ORDER; lpfilt[i++] = 0);
		for (i = 0; i < MAX_OFILT_ORDER; ofilt[i++] = 0);
		sigpos = 0;
		lppstep = LP_F0_STEP * v->inv_sampling_rate / 1000;
		lpestep = LP_DECIM * lppstep;
		basef0 = v->init_f;
		lppitch = v->inv_sampling_rate / basef0;;
	}

	// init the smoothing filter
	if (v->f0_smoothing) {
		int i;
		for (i = 0; i < MAX_OFILT_ORDER; smoothfilt[i++] = 0);
		// CHECKME: the following two lines may be superfluous
		basef0 = v->init_f;
		lppitch = v->inv_sampling_rate / basef0;
	}
}

tdpsyn::~tdpsyn(void)
{
	free(out_buff);
	free(wwin);
	unclaim(tdi);
}

inline int tdpsyn::average_pitch(int offs, int len)
{
	// const int npitch = 145;
	const int npitch = 580;
	int tmp;

	int total = 0;
	int i = 0;
	for (int j = 0; j <= len + 1; j++) {
		tmp = ppulses[offs + j] - ppulses[offs + j - 1];
		if (tmp < 2 * npitch) {
			total += tmp;
			i++;
		}
	}
	if (i <= 0) {
		if (cfg->paranoid) shriek(463,"pitch marks not found");
		return 160;
	}
	return total / i;
}

void tdpsyn::synseg(voice *v, segment d, wavefm *w)
{
	int i, j, k, l, m, slen, nlen, pitch, avpitch, origlen, newlen, maxwin, skip, reply, diflen;
	double outf0, synf0, exc;
	static double old_exc = 0;
	SAMPLE poms;
	
	const int max_frame = this->max_frame;
	int pitch_saved = 0;
	int segment_pitch;

	if (diph_len[d.code] == 0) {
		D_PRINT(2, "missing speech unit No: %d\n", d.code);
		if (!cfg->paranoid) return;
		shriek(463, "missing speech unit No: %d\n",d.code);
	}

	/* lp prosody reconstruction filter excitation signal computing */
	if (v->lpcprosody) { 	// in d.f is excitation signal value

	  // impuls at start of each sentence clears the lpc filter
		if (d.e >= v->init_i * 9) {
			for (i = 0; i < LPC_PROS_ORDER; lpfilt[i++] = 0);
			d.e = (d.e * 100 / v->init_i - 1000) * v->init_i / 100;
		}

	  if (v->bang_nnet) {
	    // multiply with 13.4 if f0 filter is used
	    exc = ((float) d.f) / 1000;
	    // exc = (int) (((float) d.f) * 13.4);
	    
	    // delete excitation if it was the same (do not allow more than one exc per syllable)
	    if (exc == old_exc) {
	      exc = 0;
	    }
	    else {
	      old_exc = exc;
	    }
	    D_PRINT(0, "Version lpc with nnet, exc. is %f\n", exc);
	  }
	  else {
		exc = LP_EXC_MUL * (d.f - v->init_f);
	    D_PRINT(0, "Version lpc, exc is %f\n", exc);
	  }

		pitch = lppitch;

	}
	else {				// in d.f is f0 contour value
	pitch = v->inv_sampling_rate / d.f;
	  D_PRINT(0, "Version without lpc, pitch is %d\n", pitch);
	}

	if (v->f0_smoothing) {
		segment_pitch = pitch; // remember pitch for whole segment
		if (lppitch > 0) {
			pitch = lppitch; // use pitch from last segment
		}
	}

	slen = diph_len[d.code];
	avpitch = average_pitch(diph_offs[d.code], slen);
	maxwin = avpitch + MAX_STRETCH;
	maxwin = (pitch > maxwin) ? maxwin : pitch;
	if (maxwin >= max_frame) shriek(461, "pitch too large");

	if (d.t > 0) origlen = avpitch * slen * d.t / 100; else origlen = avpitch * slen;
	newlen = pitch * slen;
	//diflen = (newlen - origlen) / slen;
	D_PRINT(0, "\navp=%d L=%d oril=%d newl=%d | %d (%d)\n",avpitch,pitch,origlen,newlen,diph_len[d.code],d.code);
	D_PRINT(0, "unit:%4d f=%3d i=%3d t=%3d - pitch=%d\n",d.code,d.f,d.e,d.t,pitch);

	hamkoe(2 * maxwin + 1, wwin, d.e, 100);
	skip = 1; reply = 1;
	if (newlen > origlen) skip = newlen / origlen;
	if (origlen > newlen) reply = origlen / newlen;
	D_PRINT(0, "dlen=%d p:%d avp=%d oril=%d newl=%d difl=%d",diph_len[d.code],pitch,avpitch,origlen,newlen,diflen);
	nlen = slen - (skip - 1) * slen / skip + (reply - 1) * slen;
	if (nlen == 0) {
	  D_PRINT(0, "Error: Pitch modelling exceeds range!\n");
	  nlen = 1;
	}
	diflen = (newlen - origlen - (skip - 1) * slen * pitch / skip + (reply - 1) * slen * pitch) / nlen;
	D_PRINT(0, " -> diflen:%d sk:%d rp:%d\n",diflen,skip,reply);
	for (j = 1; j <= diph_len[d.code]; j += skip) for (k = 0; k < reply; k++) {
		memcpy(out_buff + max_frame - pitch, out_buff + max_frame, pitch * sizeof(*out_buff));
		memset(out_buff + max_frame, 0, max_frame * sizeof(*out_buff));
		for (i = -maxwin;i <= maxwin; i++) {
			int ttemp;
			poms = tdp_buff[i + ppulses[diph_offs[d.code] + j - 1]];
			ttemp = wwin[i + pitch];
			ttemp *= poms;
			ttemp = ttemp >> HAMMING_PRECISION;
			poms = (SAMPLE) ttemp;
			//			poms = (SAMPLE)(wwin[i + pitch] * poms >> HAMMING_PRECISION);
//			poms = poms * d.e / 100;
			out_buff[max_frame + i] += poms;
		}

		/* lpc synthesis of F0 contour */
		if (v->lpcprosody) {
			synf0 = 0; outf0 = 0;
			for (l = 0; l < 2 * pitch; l++) {
				sigpos++;
				if (!(sigpos % lppstep)) {	// new pitch value into f0 output filter
					D_PRINT(0, "LPP position point %d, exc=%.2f synf0=%d otf0=%.2f L=%d\n",sigpos,exc,synf0,outf0,lppitch);
					synf0 = 0;
					if (!(sigpos % lpestep)) {	// new excitation value into recontruction filter
						D_PRINT(0, "   >> LPE position point %d (exc=%.4f) <<   \n",sigpos,exc);
						D_PRINT(0, "lp=[%.3f %.3f %.3f %.3f] lpfilt=[%.3f %.3f %.3f %.3f]\n",lp[0],lp[1],lp[2],lp[3],lpfilt[0],lpfilt[1],lpfilt[2],lpfilt[3]);
						synf0 = exc - lpfilt[0]*lp[0];
						exc = 0;
						for (m = LP_F0_ORD - 1; m > 0; m--) {
							synf0 -= lp[m] * lpfilt[m];
							lpfilt[m] = lpfilt[m-1];
						}
						lpfilt[0] = synf0;
						pitch_saved = 1;
						D_PRINT(0, "Unsmoothed synf0 is: %f\n", synf0);
					}

					// skip f0 filter here
					if (! (v->bang_nnet)) {
					// if (1) {
					ofilt[0] = synf0;
					synf0 = 0;
					for (m = 1; m < F0_FILT_ORD; m++) ofilt[0] -= a[m] * ofilt[m];
					outf0 = 0;
					for (m = 0; m < F0_FILT_ORD; m++) outf0 += b[m] * ofilt[m];
					D_PRINT(0, "of=[%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f]\n",ofilt[0],ofilt[1],
						ofilt[2],ofilt[3],ofilt[4],ofilt[5],ofilt[6],ofilt[7],ofilt[8]);
					  
					  // amplify the signal
					  // outf0 = outf0 * 13.4;
					}
					else {
					  outf0 = lpfilt[0];
					}

					// apply the prosody factor
					// 1000 = prosody unchanged
					// > 1000 = prosody enhanced
					// < 1000 = prosody lowered
					outf0 = outf0 * ((double) cfg->pros_factor) / 1000.0;
					
					// do remember the first output value from the lpc filter 
					if (pitch_saved == 1) {
					  pitch_saved = 2;
					lppitch = (int)(v->inv_sampling_rate / (basef0 + outf0));
					}
					D_PRINT(0, "New values: outf0 : %f, lppitch : %d\n", outf0, lppitch);
					outf0 = 0;
					for (m = F0_FILT_ORD - 1; m > 0; m--) ofilt[m] = ofilt[m - 1];
				}
			}
		}

		D_PRINT(0, "Pitch: %d\n", pitch);

		w->sample((SAMPLE *)out_buff + max_frame - pitch, pitch);
		D_PRINT(0, "  j:%d difpos:%d diflen:%d",j,difpos,diflen);
		difpos += diflen;
		if (difpos < -pitch) {
			if (reply == 1) j--; else k--;
			difpos += pitch;
		}
		if (difpos > pitch) {
			if (reply == 1) j++;
			else if (k == reply - 1) { j++; k = 1; }
			else k++;
			difpos -= pitch;
		}
		D_PRINT(0, " -> j:%d difpos:%d\n",j,difpos);

		// inserted to label f0 in wav file
		if (cfg->label_f0) {
			
			D_PRINT(0, "Labelling file!\n");
			
			char tmp[4];
			snprintf (tmp, 4, "%d", pitch);
			tmp[3] = 0;
			w->label(0, tmp, "pitch");
		}

		// f0 smoothing, when chosen
		if (v->f0_smoothing) {

			smoothfilt[0] = segment_pitch;

			for (m = 1; m < F0_FILT_ORD; m++) {
				smoothfilt[0] -= a[m] * smoothfilt[m];
			}
			
			lppitch = 0;
			for (m = 0; m < F0_FILT_ORD; m++) lppitch += b[m] * smoothfilt[m];
			
			for (m = F0_FILT_ORD - 1; m > 0; m--) smoothfilt[m] = smoothfilt[m - 1];

			if (lppitch > 0) 
				pitch = lppitch;

			slen = diph_len[d.code] - j + 1;
			avpitch = average_pitch(diph_offs[d.code] + j - 1, slen);
			maxwin = avpitch + MAX_STRETCH;
			maxwin = (pitch > maxwin) ? maxwin : pitch;
			if (maxwin >= max_frame) shriek(461, "pitch too large");
		
			if (d.t > 0) origlen = avpitch * slen * d.t / 100; else origlen = avpitch * slen;
			newlen = pitch * slen;
			
			hamkoe(2 * maxwin + 1, wwin, d.e, 100);
			skip = 1; reply = 1;
			if (newlen > origlen) skip = newlen / origlen;
			if (origlen > newlen) reply = origlen / newlen;
			
			nlen = slen - (skip - 1) * slen / skip + (reply - 1) * slen;
			if (nlen == 0) {
				D_PRINT(0, "Error: Pitch modelling exceeds range!\n");
				nlen = 1;
			}
			diflen = (newlen - origlen - (skip - 1) * slen * pitch / skip + (reply - 1) * slen * pitch) / nlen;
		}
	}
}


void tdioven(char *p, int l){
	if (!scfg->_big_endian)
		return;
	/* Convert the header */
	tdi_hdr* hdr = (tdi_hdr*)p;
	hdr->samp_rate = from_le32s(hdr->samp_rate);
	hdr->samp_size = from_le32s(hdr->samp_size);
	hdr->bufpos = from_le32s(hdr->bufpos);
	hdr->n_segs = from_le32s(hdr->n_segs);
	hdr->diph_offs = from_le32s(hdr->diph_offs);
	hdr->diph_len = from_le32s(hdr->diph_len);
	hdr->res1 = from_le32s(hdr->res1);
	hdr->res2 = from_le32s(hdr->res2);
	hdr->ppulses = from_le32s(hdr->ppulses);
	hdr->res3 = from_le32s(hdr->res3);
	hdr->res4 = from_le32s(hdr->res4);
	
	/* Convert the buffer */
	SAMPLE *bufS = (SAMPLE *)(hdr + 1);
	uint32_t *bufL = (uint32_t *)(bufS + hdr->bufpos);
	char *stop = p + l;
	
	for ( ; bufS < (SAMPLE *)bufL; bufS++)
		*bufS = from_le16s(*bufS);
	
	for ( ; bufL < (uint32_t*)stop; bufL++)
		*bufL = from_le32u(*bufL);
}

