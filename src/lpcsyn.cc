/*
 *	epos/src/lpcsyn.cc
 *	(c) 1994-2000 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
 *	(c) 1997-2000 Jirka Hanika, geo@cuni.cz
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
 */

#include "epos.h"
#include "lpcsyn.h"
#include "endian_utils.h"

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifndef   O_BINARY
#define   O_BINARY	0
#endif

//#pragma warn -pia

// DEEM, HPF, hilbk  were long int - geo

const int DEEM=29184;	// constant of deemphasis = 0.89 * 32768
const int HPF=26542;   // constant of high-pass output filter
const int UPRAV=5120;
const int nhilb=15;         // number of Hilbert coefficients of excitation impulse
const int rad=8;            // order of LPC

const int hilbk[15]={4608,0,3584,0,6656,0,20992,0,-20992,0,-6656,0,-3584,0,-4608};
                        // Hilbert impulse for voiced excitation
const int lroot=10054;

const int minsynt=23;


struct cmodel
{
	int16_t  rc[8];
	int16_t  ener;
	int16_t  incrl;
};               // model for integer synthesis

struct fcmodel
{
	float rc[8];
	float ener;
	int32_t   incrl;
};              // model for float synthesis

struct vqmodel
{
	int16_t  adrrc;
	int16_t  adren;
	int16_t  incrl;
};              // model for vector quantised synthesis


lpcsyn::lpcsyn(voice *v)
{

	int i; //,delmod;

	kyu = 0;		/* random initial value - geo */

	seg_offs = (int *)xmalloc(sizeof(int) * v->n_segs);
//	seg_len = (unsigned char *)xmalloc(sizeof(unsigned char) * v->n_segs);

	nvyrov = 0;
	lold = v->init_f;

/* incipit constructor clasae synth */
	ipitch = 0;
	lastl = lold;
	iyold = 0 ;

	for (i=0; i<rad; i++) ifilt[i] = 0;

//	tseg = (char (*)[4])freadin(v->dptfile, v->inv_dir, "rt", "segment names");
	seg_len = claim(v->counts, v->location, scfg->inv_base_dir, "rb", "model counts", NULL);
	seg_offs[0] = 0;
	for (i = 1; i < v->n_segs; i++)
		seg_offs[i] = seg_offs[i-1] + (int)seg_len->data[i-1];	// cast to unsigned char * before [] if LPC sounds bad
//	models = claim(v->models, v->location, scfg->inv_base_dir, "rb", "lpc inventory");
//	delmod = seg_offs[440] + seg_len[440];
}

lpcsyn::~lpcsyn(void)
{
	if (seg_offs) free(seg_offs);
	if (seg_len) unclaim(seg_len);
	if (models) unclaim(models);
}


/*
 *	FIX the following. If ipitch>0, mod.lsyn>0, and
 *	ipitch+mod.lsyn-lastl!=0, then ihilb is used before inited.
 *
 *	If this can't ever happen, remove ihilb = 0 as well as the shriek.
 *	Else remove ihilb = 0 and fix the warning which results.
 *
 */

inline void lpcsyn::synmod(model mod, wavefm *w)
{
	int i,k,ihilb = 0;
	int gain,finp,iy,jrc[8],kz;	// were long - geo

	D_PRINT(1, "lsyn=%d nsyn=%d esyn=%d lsq=%d\n", mod.lsyn, mod.nsyn, mod.esyn, mod.lsq);

	if (cfg->paranoid && ipitch>0 && mod.lsyn>0 && ipitch + mod.lsyn - lastl)
		shriek(862, "ihilb is undefined in synteza.synmod! Please contact the authors.\n");
	for(i=0;i<rad;i++) jrc[rad-i-1]=(int)mod.rc[i];		// cast was to long - geo
	gain=(int)mod.lsq;		// cast was to long - geo
	if(mod.lsyn!=0)
	{
		if(ipitch != 0 && ipitch + mod.lsyn - lastl>0) ipitch += mod.lsyn - lastl;
	} else 	ipitch = 0;
	if (lastl == 0 && mod.lsyn != 0) for (i=0; i<rad; i++) ifilt[i]=0;

	for(k=0;k<mod.nsyn;k++)
	{	//buzeni
		if(mod.lsyn==0) finp = rand()%(2*UPRAV)-UPRAV;
		else {
			finp=0;
			if (ipitch==0) {
				ipitch=mod.lsyn; ihilb=0; finp=hilbk[ihilb++];
			} else if (ihilb<nhilb) finp=hilbk[ihilb++];
		}
		//kriz_clanek
		iy=finp*gain-jrc[0]*ifilt[0];
		for(i=1;i<rad;i++) {
			iy=iy-jrc[i]*ifilt[i];
			ifilt[i-1]=(((iy>>15)*jrc[i])>>15)+ifilt[i];
		}
		ifilt[rad-1]=(int)(iy>>15);
		//vyst_uprav
		iy=(iy>>15)*(int)mod.esyn+(DEEM*iyold>>4);	// cast was to long - geo
		iyold=iy>>11;
		kz=kyu+iy;
		kyu=-iy+(kz>>15)*HPF;

		if (mod.lsyn!=0) ipitch--;

		w->sample(kz>>11);
	}
	lastl = mod.lsyn;
}//synmod


void lpcsyn::synseg(voice *v, segment d, wavefm *w)	// voice not used
{
	int i,imodel,lincr,incrl,numodel,zaklad,znely;
	model m;
//	if (cfg->ti_adj) shriek("Should call lpcvq::adjust(segment *), but can't"); // twice in this file
	D_PRINT(1, "lpcsyn processing another segment\n");
//	d.eproz=(d.eproz-100) / 9 + (cfg->ti_adj ? kor_i[d.hlaska]-15 : 0);
	lincr=0;
	if (d.code > v->n_segs) shriek(463, "Segment number %d occurred, but the maximum is %d\n", d.code, v->n_segs);
	numodel=(int)seg_len->data[d.code];	// cast to unsigned char * before [] if problems
	if(!numodel) {
		D_PRINT(2, "Unknown segment %d, %3s\n", d.code, d.code < v->n_segs ? "in range" : "out of range");
		return;
	}
	/* nacti_mem_popis(code,numodel) */

	for (imodel=0; imodel<numodel; imodel++) {
		// nacti_k_model(imodel)

		frobmod(imodel, d, &m, incrl, znely);

		// synchr(imodel)
		if(!znely) lincr += incrl;
		if(nvyrov < d.t / 2) {
			if(!znely) m.lsyn=0, m.lsq=12288, m.nsyn=d.t-nvyrov;
			else {
				if(imodel==numodel-1) m.lsyn = v->inv_sampling_rate / d.f;
				else {
					zaklad = (v->inv_sampling_rate / d.f - lold) * 256 / numodel;
					m.lsyn = lold + zaklad*(imodel+1) / 256;
				}
				m.lsyn=m.lsyn+lincr;
				m.lsq=lroot;
				i = d.t / m.lsyn + 1;
				if(i==1)m.nsyn=m.lsyn; else
					if(abs(nvyrov-d.t+i*m.lsyn)<abs(nvyrov-d.t+(i-1)*m.lsyn))m.nsyn=i*m.lsyn;
					else m.nsyn=(i-1)*m.lsyn;
			}
			nvyrov += m.nsyn - d.t;
		} else {
			if(imodel!=numodel-1) m.nsyn=0, m.lsyn=0, m.lsq=12288;
			else if(!znely) m.lsyn=0, m.lsq=12288, m.nsyn=minsynt;
				else m.lsyn = v->inv_sampling_rate / d.f, m.lsq=lroot, m.nsyn=m.lsyn;
			nvyrov -= d.t;
		}
		D_PRINT(1, "Model %d\n", imodel+1);
		if(m.nsyn>=minsynt) synmod(m, w);  //proved syntezu modelu
	}
	lold = v->inv_sampling_rate / d.f;
}//hlask_synt


void lpcvq::frobmod(int imodel, segment d, model *m, int &incrl, int &znely)
{
	int i;
	vqmodel *vqm = (vqmodel *)models->data + seg_offs[d.code] + imodel;
	int16_t (*cbook)[8] = (int16_t (*)[8])codebook->data;

	incrl = vqm->incrl == 999 ? 0 : -vqm->incrl;
//	if(vqmodels[seg_offs[d.code]+imodel].incrl == 999) incrl = 0;
//	else incrl = -vqmodels[seg_offs[d.code]+imodel].incrl;
	znely = (vqm->incrl != 999);	// was "=" instead of "!=" (a bug?) - geo
	d.e = (d.e-100) / 9; // + (cfg->ti_adj ? kor_i[d.code]-15 : 0);
	i = vqm->adren-1 + d.e;
	if (i>63) i=63;                    //uprava indexu
	if (i<0) i=0;                      //(tabulka energii ma jen 64 poli)
	D_PRINT(1, "energeia %d\n", i);
	m->esyn = ener[i];                   //vyber energii z tabulky
	for (i=0; i<rad; i++) m->rc[i] = cbook[vqm->adrrc-1][i];
}


void lpcfloat::frobmod(int imodel, segment d, model *m, int &incrl, int &znely)
{
	int i;
	fcmodel *fcm = (fcmodel *)models->data + seg_offs[d.code] + imodel;

	incrl = fcm->incrl;
	m->esyn = (int)(32768.0 * fcm->ener);
	m->esyn = m->esyn * d.e / 100;
	for (i=0; i<rad; i++)
		m->rc[i]=(int)(32768.0 * fcm->rc[i]);
	znely = (incrl!=999);
	if (incrl == 999) incrl = 0;
}

void lpcint::frobmod(int imodel, segment d, model *m, int &incrl, int &znely)
{
	int i;
	cmodel *cm = (cmodel *) models->data + seg_offs[d.code] + imodel;

//  	if (cfg->ti_adj) adjust(d);
	incrl = (int)cm->incrl;
	m->esyn = (int)cm->ener;
	m->esyn = m->esyn * d.e / 140;
	for(i=0; i<rad; i++)
		m->rc[i] = cm->rc[i];
	znely = (incrl != 999);
	if (incrl == 999) incrl = 0;
}

void floatoven(char *p, int l)
{
	if (!scfg->_big_endian)
		return;
	char*   stop = p + l;
	for (fcmodel* tmp = (fcmodel*)p; (char *)tmp < stop; tmp++){
		for (int i = 0;i < 8; i++) {
			tmp->rc[i] = (float) from_le32u(tmp->rc[i]);
		}
		tmp->ener = (float) from_le32u(tmp->ener);
		tmp->incrl = from_le32s(tmp->incrl);
	}
}

void intoven(char *p, int l)
{
	if (!scfg->_big_endian)
		return;
	char*   stop = p + l;
	for (cmodel* tmp = (cmodel*)p; (char*)tmp < stop; tmp++){
		for (int i = 0; i < 8; i++) {
			tmp->rc[i] = from_le16s(tmp->rc[i]);
		}
		tmp->ener = from_le16s(tmp->ener);
		tmp->incrl = from_le16s(tmp->incrl);
	}	
}

void vqoven(char *p, int l)
{
	if (!scfg->_big_endian)
		return;
	char*   stop = p + l;
	for(vqmodel* tmp = (vqmodel*)p; (char*)tmp < stop; tmp++){
		tmp->adrrc  = from_le16s(tmp->adrrc);
		tmp->adren  = from_le16s(tmp->adren);
		tmp->incrl  = from_le16s(tmp->incrl);
	}	
}

void shortoven(char *p, int l)
{
	if (!scfg->_big_endian)
		return;
	char *stop = p + l;
	for(int16_t *tmp = (int16_t *)p; (char *)tmp < stop; tmp++)
		*tmp = from_le16s(*tmp);
}


lpcfloat::lpcfloat(voice *v) : lpcsyn(v)
{
// 	fcmodely = (fcmodel *)freadin(v->models, v->inv_dir, "rb", "float inventory");
	models = claim(v->models, v->location, scfg->inv_base_dir, "rb", "lpc inventory", floatoven);
}

lpcint::lpcint(voice *v) : lpcsyn(v)
{
// 	cmodely = (cmodel *)freadin(v->models, v->inv_dir, "rb", "integer inventory");
#if 0
	if (cfg->ti_adj) { //Needs some more hacking before use -- twice in this file
//	    kor_t = (int16_t *)freadin("korekce.set", v->inv_dir, "rb", "integer inventory");
		shriek(462, "Remove the comment above this message and adapt the line to claim()/unclaim(), then remove this shriek()");
	    kor_i = (int16_t *)(v->n_segs*2 + (char *)kor_t);
	}
#endif
	models = claim(v->models, v->location, scfg->inv_base_dir, "rb", "lpc inventory", intoven);
}

lpcvq::lpcvq(voice *v) :lpcsyn(v)
{
	codebook = claim(v->codebook, v->location, scfg->inv_base_dir, "rb", "vector quant synthesis codebook", shortoven);
	ener = (int16_t *)(256*16 + (char *)codebook->data);
	models = claim(v->models, v->location, scfg->inv_base_dir, "rb", "lpc inventory", vqoven);
}

lpcvq::~lpcvq()
{
	unclaim(codebook);
}

void lpcint::adjust(segment d)
{
	int pomf;
	pomf=d.t;                         //korekce zatim nepouzivam
	pomf=kor_t[d.code]*pomf;
	d.t=pomf / 100;
	d.e=d.e*(100+(kor_i[d.code]-15))/100;  //Help Ptacek
//	d.e=d.e*(10000-625*(15-kor_i[d.code]));  //Checkme
}

