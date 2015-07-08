/** @file perceptron.cpp

  @brief CPerceptronNN functions needed for network running 
	(in the non-training mode)

  @author Jakub Adámek
  Last modified $Id: perceptron.cc,v 1.22 2002/04/23 15:49:41 jakubadamek Exp $
*/

#include "perceptron.h"
#include "vector.h"
#include "base.h"

#include <time.h>
#include <math.h>
#include <sys/timeb.h>

REGSMALLVECTOR (double, CFloat, TSmallVectorFloat)
REGVECTOR (CFloat, TVectorFloat) 
REGVECTOR (CInt, TVectorInt) 

REGVECTOR (TPerceptron, TVectorPerceptron)
REGVECTORC (CPerceptronNN, TConnectionsAttribs, TConAttribs)
REGVECTOR (TVectorPerceptron, TSeriesNeurons);

REGSTRUCT (TLearningGoal)
REGSTRUCT (CPerceptronNN)
REGSTRUCT (TPerceptron)
REGSTRUCTC (CPerceptronNN, TConnectionsAttribs)
REGSTRUCTC (CPerceptronNN, TPerceptrons)

REGSTRUCT (TConIndex)
REGSET (TConIndex, TConIndexes)

CPerceptronNN * globalDebugNN;

void CPerceptronNN::runNetwork (const TSmallVectorFloat &inputs, int irow)
{
	TPerceptron * nStart, * nEnd=NULL, * noldEnd=NULL;
	int ineu, con;
	TVectorPerceptron *last = NULL;
	if (irow > -1) last = &seriesNeurons[irow];

	globalDebugNN = this;
	
	for (ineu=0; ineu < (int)layerSizes[0]; ++ineu) 
		neurons[ineu].output = inputs[ineu];
	for (; ineu < neurons.size(); ++ineu)
		neurons[ineu].output = 0;

	while (next_connection (nStart,nEnd,con,last)) {
		if (noldEnd == NULL) noldEnd = nEnd;
		else if (noldEnd != nEnd) {
			noldEnd->output = noldEnd->transfer();
			noldEnd = nEnd;
		}
		nEnd->output += nStart->output * weights->d[con]; //(**weights.d).d.d[con] into watch window
	}
	if (nEnd) nEnd->output = nEnd->transfer();

	if (irow > -1) {
		if (seriesNeurons.size() <= irow+1) 
			seriesNeurons.push_back (seriesNeurons[0]);
		for (ineu = 0; ineu < neurons.size() - layerSizes[0]; ++ineu) {
			seriesNeurons[irow+1][ineu].output = neurons[ineu+layerSizes[0]].output;
			seriesNeurons[irow+1][ineu].partialEk = 0;
		}
	}
}

void CPerceptronNN::copyOutputs (TSmallVectorFloat &outputs) const
{
	for (int ineu = outOffset; ineu < neurons.size(); ++ineu)
		outputs[ineu-outOffset] = (double)neurons[ineu].output;
}

double CPerceptronNN::getError (const CTrainingData *trData, CInt set, bool postprocessed) 
{
	int ineu;
	int irow, series;

	if (trData) {
		for (ineu=outOffset; ineu < neurons.size(); ++ineu)
			neurons[ineu].error = 0;
		trData->moveToSetStart(set);
		while (trData->getSet() == set) {
			irow = 0;
			series = trData->getSeries();
			while (series == trData->getSeries()) {
				runNetwork (trData->getInputs(), irow);
				++irow;
				for (ineu=outOffset; ineu < neurons.size(); ++ineu) 
					neurons[ineu].error += sqr (neurons[ineu].output - trData->getOutputs()[ineu-outOffset]);
				trData->moveNext (set);
			}
		}
	}

	double retVal = 0;
	double val, val2;
	for (ineu=outOffset; ineu < neurons.size(); ++ineu) {
		if (postprocessed && trData) {
			val2 = sqrt (neurons[ineu].error / trData->getRowCount((enumSetType)(int)set));
			trData->getColumn(CU_OUTPUT,ineu-outOffset).postprocessError (val2,val);
			retVal += val;
		}
		else retVal += neurons[ineu].error / 2;
	}
	return retVal;
}


void CPerceptronNN::calculateOutputs (CTrainingData &trData, CString fileName) 
{
	int ineu;
	int irow, series;

	for (ineu=outOffset; ineu < neurons.size(); ++ineu)
		neurons[ineu].error = 0;
	for (int s=0; s < 2; ++s) {
		enumSetType set = s == 0 ? ST_TRAIN : ST_EVAL;
		trData.moveToSetStart(set);
		while (trData.getSet() == set) {
			irow = 0;
			series = trData.getSeries();
			while (series == trData.getSeries()) {
				runNetwork (trData.getInputs(), irow);
				++irow;
				for (ineu=outOffset; ineu < neurons.size(); ++ineu) 
					neurons[ineu].error += sqr (neurons[ineu].output - trData.getOutputs()[ineu-outOffset]);
				copyOutputs (trData.modifyOutputs());
				trData.moveNext (set);
			}
		}
	}
	CString data;
	trData.dumpData (data);
	if (fileName.length()) {
		bang_ofstream dump (fileName);
		if (dump) dump << ((const char*)data);
	}
}

void CPerceptronNN::initConAttribs ()
{
	int conCount = 0;
	int ilayer;
	CInt iStart;
	int neuronCount = 0;
	TConIndex conIndex;

	TVectorInt layerStarts (layerSizes.size()+1);
	layerStarts[0] = 0;
	for (ilayer=1; ilayer <= layerSizes.size(); ++ilayer)
		layerStarts[ilayer] = layerStarts[ilayer-1] + layerSizes[ilayer-1];

	for (ilayer = 0; ilayer < layerSizes.size(); ++ilayer) 
		neuronCount += (int)layerSizes[ilayer];
	
	conIndexes.DeleteAll();
	conIndex.recurrent = 0;

	switch ((int)conRestrict) {
	case CR_LAYERED:
		for (ilayer = 1; ilayer < layerSizes.size(); ++ilayer) 
			for (conIndex.end = layerStarts[ilayer]; conIndex.end < layerStarts[ilayer+1]; ++conIndex.end) {
				neurons[conIndex.end].inputCount = layerSizes[ilayer-1] + 1;
				conIndex.start = -1;
				conIndexes.insert (conIndex);
				for (conIndex.start = layerStarts[ilayer-1]; conIndex.start < layerStarts[ilayer]; ++conIndex.start) 
					conIndexes.insert (conIndex);
			}
		break;
	// connections lead from all neurons including bias to all hidden and output neurons
	default: 
		for (conIndex.end = layerSizes[0]; conIndex.end < (CInt)neuronCount; ++conIndex.end) {
			if (conIndex.end < layerStarts[1]) iStart = 0;
			else iStart = layerStarts[0];
			neurons[conIndex.end].inputCount = conIndex.end - iStart + 1;
			conIndex.start = -1;
			conIndexes.insert (conIndex);
			for (conIndex.start = iStart; conIndex.start < conIndex.end; ++conIndex.start)
				conIndexes.insert (conIndex);
		}
	}

	// RECURRENT CONNECTIONS:

	conIndex.recurrent = 1;
	CInt from[2], to[2], sfrom[2], sto[2];
	TAllRecurrentConnections::iterator recs;
	for (recs = allRecurrentConnections.begin(); recs != allRecurrentConnections.end(); ++recs) {
		sfrom[0] = recs->start.from; sfrom[1] = recs->end.from;
		sto[0] = recs->start.to; sto[1] = recs->end.to;
		setPercent (sto[0], layerSizes[recs->start.layer] - 1);
		setPercent (sfrom[0], layerSizes[recs->start.layer] - 1);
		setPercent (sto[1], layerSizes[recs->end.layer] - 1);
		setPercent (sfrom[1], layerSizes[recs->end.layer] - 1);
		from[0] = sfrom[0] + layerStarts[recs->start.layer];
		to[0] = Min/*<int>*/ ((int) (sto[0] + layerStarts[recs->start.layer]),(int)( neuronCount-1 ));
		from[1] = sfrom[1] + layerStarts[recs->end.layer];
		to[1] = Min/*<int>*/ ((int) (sto[1] + layerStarts[recs->end.layer]),(int)( neuronCount-1 ));

			// irix CC doesn't allow the type info after Min

		for (conIndex.end = from[1]; conIndex.end <= to[1]; ++conIndex.end) {
			neurons[conIndex.end].inputCount += to[0] - from[0] + 1;
			for (conIndex.start = from[0]; conIndex.start <= to[0]; ++conIndex.start)
				conIndexes.insert (conIndex);
		}
	}

	switch ((int) trainProcedure) {
	case TP_STABLE_CONJUGATE: conAttribs.Realloc (6); break;
	case TP_RUN_ONLY: conAttribs.Realloc (1); break;
	default: conAttribs.Realloc (4);
	}

	int iAttr = 0;
	for (TConAttribs::iterator citer=conAttribs.begin(); citer != conAttribs.end(); ++citer) {
		switch (iAttr++) {
		case 0: weights = citer; weights->name = "weights"; break;
		case 1: gradient = citer; gradient->name = "gradient"; break;
		case 2: bestWeights = citer; bestWeights->name = "bestWeights"; break;
		case 3: D = citer; D->name = "D"; break;
		case 4: oldGradient = citer; oldGradient->name = "oldGradient"; break;
		case 5: oldD = citer; oldD->name = "oldD"; break;
		}
		citer->d.Realloc (conIndexes.size());
	}
}

void CPerceptronNN::initNeurons ()
{
	if (debug == 0) 
		srand( (unsigned)time( NULL ) );

	int neuronCount = 0;
	int ilayer;

	for (ilayer = 0; ilayer < layerSizes.size(); ++ilayer) 
		neuronCount += (int)layerSizes[ilayer];
	outOffset = neuronCount - layerSizes[layerSizes.size()-1];
	neurons.data.Realloc (neuronCount+1);
	// bias outputs steadily 1
	neurons[-1].output = 1;

	int inbase = 0;
	for (ilayer=0; ilayer < layerSizes.size(); ++ilayer) {
		for (int ineuron=inbase; ineuron < (int)layerSizes[ilayer]+inbase; ++ineuron) {
			neurons[ineuron].layerSize = (int)layerSizes[ilayer];
			neurons[ineuron].layer = (CInt)ilayer;
			if (ilayer) 
				neurons[ineuron].transferFn = layerTrFuncs[ilayer];			
		}
		inbase += (int)layerSizes[ilayer];
	}

	int neu;
	for (neu=-1; neu < neurons.size(); ++neu)
		neurons[neu].index = (CInt)neu;

	seriesNeurons.Realloc (1);
	seriesNeurons[0].Realloc (neurons.size() - layerSizes[0]);
	for (neu = 0; neu < neurons.size() - layerSizes[0]; ++neu) {
		seriesNeurons[0][neu] = neurons[neu+layerSizes[0]];
		seriesNeurons[0][neu].output = 0;
		seriesNeurons[0][neu].partialEk = 0;
	}
}
		
CXml * CPerceptronNN::printWeights (const CString &filename) const
{
	TStream str;
	if (dumpWeights.length()) {
		str.type = ST_REMOTE;
		str.value = findFileName (getFilePath(filename)+dumpWeights);
	}
	else str.type = ST_LOCAL;
	str.encoding = SE_TEXT;
	str.delimiter = dumpWeightsDelimiter;
	return printStream (str, weights->d.begin(), weights->d.end(), "");
} 

CString CPerceptronNN::readWeights (CRox &stream, const CString &filename)
{	
	assert (weights.valid());
	if (!weights.valid()) return "PerceptronNN: Can't read weights: not yet initialized.";
	TStream str;
	return readStream (str, stream, weights->d, filename); 
}

CString CPerceptronNN::read (CRox *xml)
{
	return TPerceptronStructure::read (xml);
}

CXml * CPerceptronNN::print () const
{
 	return TPerceptronStructure::print ();
}
