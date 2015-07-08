/** @file percinit.cpp

  @brief CPerceptronNN weights initialization

  @author Jakub Adámek
  Last modified $Id: percinit.cc,v 1.2 2002/04/19 17:44:55 jakubadamek Exp $
*/

#include "perceptron.h"
#include "utils.h"

#include <time.h>
#include <math.h>
#include <sys/timeb.h>

/** returns the sum distribution of Gauss normal distribution with average = 0,
	stdError = 1, for a given probability 0..1 */

double normalDistribution (double average, double variance) 
{
	double probability = double(rand())/RAND_MAX;

	// Table with distributions 0.05,0.1,0.15,0.2,...0.5
	// for probability > 0.5 the result is -normalDist(1-prob)
	double tab [10] = 
		{ -1.64 /*0,05*/,-1.28/*0.1*/,-1.04,-0.84,-0.67,-0.52,-0.39,-0.25,-0.13,0 /*0.5*/};
	
	bool big = probability > 0.5;
	if (big) probability = 1 - probability;
	int index = int (((probability + 0.0025) * 20) - 1);
	if (index == -1) index = 0;
	double retVal = big ? -tab[index] : tab[index];
	return retVal * variance + average;
}

void CPerceptronNN::weightInitNguyenWidrow ()
{
	// region in which the transfer function is not saturated
	double activeRegion = 2, 
		// max and min value of input signals
		xmax, xmin;
	// overlapping factor
	double kapa = 0.7;
	int inpmin, inpmax, inp, layer, il;
	TVectorFloat a;
	double norm;
	for (int neu=layerSizes[0]; neu < neurons.size(); ++neu) {
		if (neu < layerSizes[1]) 
			{ xmin = -1; xmax = 1; }
		else { xmin = 0; xmax = 1; }
		layer = neurons[neu].layer;
		inpmin = 0;
		for (il = 0; il < layer-1; ++il)
			inpmin += layerSizes[il];
		inpmax = inpmin + layerSizes[il];
		a.Realloc (inpmax - inpmin);
		norm = 0;
		for (inp=0; inp < inpmax - inpmin; ++inp) {
			a[inp] = 2.0 * double(rand())/RAND_MAX - 1;
			norm += sqr(a[inp]);
		}
		norm = sqrt (norm);
		for (int inp=inpmin; inp < inpmax; ++inp)
			if (weights->d[getConnection(inp,neu)])
			weights->d[getConnection(inp,neu)] = 
				(a[inp-inpmin] / norm) * activeRegion * kapa 
				* pow (layerSizes[layer],1/double(layerSizes[layer-1]))
				/ (xmax - xmin);
		xmin = -1 - xmax * weights->d[getConnection(inpmin,neu)];
		xmax = 1 - xmin * weights->d[getConnection(inpmin,neu)];
		if (weights->d[getConnection(-1,neu)])
			weights->d[getConnection(-1,neu)] = (xmax-xmin)*double(rand()) / RAND_MAX + xmin;
	}
}

void CPerceptronNN::weightInit ()
{
	TPerceptron * nStart, * nEnd=NULL;
	int con;
	
	for (con=0; con < weights->d.size(); ++con)
		weights->d[con] = 1;

	switch ((int)weightInitProcedure) {
	case WI_NGUYEN_WIDROW: weightInitNguyenWidrow (); break;
	case WI_RANDOM:
		while (next_connection (nStart,nEnd,con,&seriesNeurons[0])) 
			weights->d[con] = normalDistribution (0,1.0/sqrt(nEnd->inputCount));
		break;
	}
}
	
