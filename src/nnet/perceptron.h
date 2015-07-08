/** @file perceptron.h

  @brief CPerceptronNN header - Perceptron Neural Network

  @author Jakub Adámek
  Last modified $Id: perceptron.h,v 1.21 2002/04/23 15:49:41 jakubadamek Exp $
*/

#ifndef __BANG_FFNN_H__
#define __BANG_FFNN_H__

#include "math.h"
#include "time.h"
#include "traindata.h"
#include "percstruct.h"

//#include "../../base/bing.h"

/// abstract neuron transfer function 
typedef double TFloat2FloatFn (double,double);

//template class TVector <CFloat, TVectorFloat_label_>; 

/**	+ tf... are neuron transfer functions,
	+ wif... are weight initialize functions */

static const int fnsCount = 3;

struct TFloat2FloatFns {
	/// "linear" : linear transfer function
	static double tfLinear (double n, double) { return n; }
	static double tfLinearDeriv (double, double) { return 1; }
	/// "logsig" : logical sigmoid transfer function
	static double tfLogSig (double n, double lambda) { return 1.0 / (1.0 + exp (-n*lambda)); }
	static double tfLogSigDeriv (double n, double lambda) { return lambda * n * (1 - n); }
	/// "tanh" : hyperbolical tangent
	static double tfTanH (double n, double) { return 2 / (1 + exp(-2*n)) - 1; }
	static double tfTanHDeriv (double n, double) { return 1 - sqr (n); }
};

/// perceptron neuron 
STRUCT_BEGIN (TPerceptron)
public:
	void Init()	{ error=0; output=0; layerSize=0; inputCount=0; layer=-1; 
			  transferFn=-1; lambda=1; }
	/// accumulates error over all trained data
	CFloat error;
	/// current output (during calculation)
	CFloat output;
	/// the second transfer fn parameter
	CFloat lambda;
	/// partial derivation of Ek
	CFloat partialEk;
	CInt transferFn;
	/// returns transfer Fn applied on output
	inline double transfer ();
	/// returns transfer function derivation needed in backpropagation
	inline double transferDeriv ();
	CInt layerSize;
	/// count of incoming connections including bias
	CInt inputCount;
	CInt layer;
	/// just for debugging reason - index in the neurons vector
	CInt index;
STRUCT_END (TPerceptron)
VECTOR (TPerceptron, TVectorPerceptron);
VECTOR (TVectorPerceptron, TSeriesNeurons);

STRUCT_BEGIN (TConIndex)
public:
	/// neuron from which the connection goes
	CInt start;
	/// neuron to which it goes
	CInt end;
	/// is it a recurrent connection?
	CInt recurrent;
	bool operator > (const TConIndex &y) const { return end > y.end; }
	bool operator < (const TConIndex &y) const { return end < y.end; }
STRUCT_END (TConIndex)

SET (TConIndex, TConIndexes)

/** @brief Encapsulates all the functionality of Perceptron Neural Network

	Neurons are indexed over layers, 0..n1-1 in the 1st layer, n1..n1+n2-1 in the 2nd etc.
	Biases are considered as weights from neuron -1, with output value = 1.
*/

STRUCT_BEGIN_FROM (CPerceptronNN, TPerceptronStructure)
public:
	friend class PerceptronNN;

	void Init()	{ weights=NULL; gradient=NULL; oldGradient=NULL; D=NULL; oldD=NULL;
			  pushGoalInterval = 0; runWeightsPerturbation=0; }

	/// this gives the possibility for bias (neuron -1) to be included
	STRUCT_BEGIN (TPerceptrons)
		TVectorPerceptron data;
		TPerceptron & operator [] (int i)	{ return data[i+1]; }
		const TPerceptron & operator [] (int i) const { return data[i+1]; }
		int size() const		{ return data.size() - 1; }
	STRUCT_END (TPerceptrons)

	/// all neurons including inputs (although inputs don't have transferFn etc.)
	TPerceptrons neurons;

	/// used to store previous neuron output values when counting time series with recurrent network
	TSeriesNeurons seriesNeurons;

	// attribute for all connections - e.g. weights. Includes a fictive connection to bias.
	STRUCT_BEGIN (TConnectionsAttribs)
		void Init()	{ name=""; permanent=false; }
		/// name which identifies this attrib
		CString name;
		/// data in one vector over all connections
		TSmallVectorFloat d;
		/** bool - should be saved to file? If it takes a long time to calculate it,
			the attrib should be permanent, otherwise not. */
		CInt permanent;
	STRUCT_END (TConnectionsAttribs)

	VECTORC (TConnectionsAttribs, TConAttribs);
	TConAttribs conAttribs;

	TConIndexes conIndexes;

	TConAttribs::iterator 
		weights,
		/** weights on which the error was lowest - when trained with eval set, 
			error of eval set is taken */
		bestWeights,
		gradient,
		/// not necessary in all algorithms
		oldGradient,
		/// stable conjugate gradient 
		D, oldD;

	const TSmallVectorFloat & getWeights () const { return weights->d; }

	/** main function - runs the whole training procedure
		return which goal was met */
	TLearningGoal train (CTrainingData *, const CString &filename);
	/** number of milliseconds between two goal writes */
	CInt pushGoalInterval;

	/// Creates and sets neurons, uses initSeriesNeurons(). 
	void initNeurons ();
	/// creates and sets connections attributes (weights etc.)
	void initConAttribs ();
	CXml * printWeights (const CString &filename) const;
	/** Reads weights from <stream> tag, returns error description or empty string.
		The filename of the config file is used to get the file path. */
	CString readWeights (CRox &stream, const CString &filename);
	/// initializes weights
	void weightInit ();
	/// initializes weights by Nguyen-Widrow - called from weightInit()
	void weightInitNguyenWidrow();

	/// initializes the weights with 1 or 0 indicating whether connection exists
	void processRecurrentConnections();
	
	/// runs given inputs through network and accumulates error
	void runNetwork (const TSmallVectorFloat &inputs, int irow = -1);
	/** runs the error back propagation to calculate the error function first derivatives */
	void backPropagate (const TSmallVectorFloat &outputs, int irow = -1);
	/** Runs the network on all input data (batch training) and calculates gradient
		by backpropagation. Works well with recurrent networks. */
	void calculateGradient (const CTrainingData *);

	/// copies neuron outputs to the given outputs
	void copyOutputs (TSmallVectorFloat &outputs) const;
	/// calculates all outputs and overwrites output in traindata, dumps the traindata
	void calculateOutputs (CTrainingData &trData, CString fileName = "");

	void putWeights (bang_ofstream & str);

	/** Returns: sum of errors on output neurons. 
		If trData supplied, iterates through all data from given set first. 
		If postprocessed==true, postprocesses the error back to the magnitude of the source data */
	double getError (const CTrainingData * trData = NULL, CInt set=ST_TRAIN, bool postprocessed = false);

//private:
	/// index of first output layer neuron
	int outOffset;
	/// error sizes
	TVectorFloat epochError;

	/** Iterates through all connections. Parameters:
		inStart - index of neuron in which connection starts
		inEnd   - dtto ends
		con	- index of connection

	    Usage: 
	    + int inStart, inEnd=-1, con;
	    + while (next_connection) do_something;

	    You are ensured that all weights for inEnd are evaluated in a row,
	    i.e. inEnd grows steadily neuron by neuron.

	    This will run terrible lots of times - make it as quickly as possible!
	*/
	inline bool next_connection (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu = NULL);

	/** Like next_connection, you are ensured inEnd decreases steadily neuron by neuron. */
	inline bool prev_connection (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu = NULL);

	inline bool next_connection_recurrent (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu);
	inline bool prev_connection_recurrent (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu);

	/** Returns "con" - index of connection between given neurons */
	inline int getConnection (int inStart, int inEnd);

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		           LEARNING ALGORITHMS
	 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

	/// special powerful algorithm by Stefan Rueger
	void stableConjugateGradient (int epoch, const CTrainingData *);
	/// Levenberg-Marquardt with modified line search by Croatian team
	void modifiedLevenbergMarquardt (int epoch, const CTrainingData *trData);
	/// the simple basic gradient descent with learning rate only
	void gradientDescent (const CTrainingData *trData);
	void gradientDescent_recurrent (const CTrainingData *trData);
	/** This is a simple numerical technique to calculate gradient. It is implemented here just
		to proove correctness of the much more efficient backPropagation technique. 
		Returns difference between this way computed gradient and the current gradient
		(which should be counted before by backPropagation) */
	double weightsPerturbation (const CTrainingData *trData);

	/** If the trData parameter is given, the training data configuration is added as a subtag */
	virtual CXml *print_all (const CString &filename, CTrainingData *trData = NULL) const;
	/** If the trData parameter is given, the training data configuration is read from a subtag */
	virtual CString read_all (CRox *xml, const CString &filename, CTrainingData *trData = NULL);

	CString start (CTrainingData *trData, const CString &filename);
	int stopCriterion ();
	CString nextIteration (CTrainingData *trData, const CString &filename);
	void end(CTrainingData *trData);

private:
	/// Jacobian matrix and error vector used in MLM method
	CMatrix<double> J, e;
	double Emin;
	int iData;

	time_t startTime;
	int lastBackupTime, lastGoalWrite;
	double bestError, currentError;
	bang_ofstream errorLog;
	/// local variables used in iterations
	TLearningGoal goalState;
	CString filename;
STRUCT_END_FROM(CPerceptronNN,TPerceptronStructure)

/** Connections are organized this way:
	[x,y] means connection x->y
	For CR_FEEDFORWARD - consider network with 62 inputs, 20 hidden units in 1st layer, 10 in 2nd, 8 outputs
	[-1,62][0,62]..[61,62] (63 connections)
	[-1,63][0,63]..[61,63] (63)
	...		       (20 times 63 cons)
	[-1,82][62,82][63,82]..[81,82] (21 connections)
	[-1,83][62,83]..[81,83] (21)
	...			(10 times 21 cons)
	[-1,92][82,92]..[91,92] (11)
	...
	[-1,99][82,99]..[91,99] (8 times 11 cons)
*/

inline int CPerceptronNN::getConnection (int inStart, int inEnd) 
{
	return 0;
	if ((int)conRestrict == CR_LAYERED) {
		int con = 0;
		int layer = neurons[inEnd].layer;
		bool bias = inStart == -1;
		int ilayer;
		for (ilayer=1; ilayer < layer; ++ilayer) {
			con += (int)layerSizes[ilayer] * ((int)layerSizes[ilayer-1]+1);
			inStart -= (int)layerSizes[ilayer-1];
			inEnd -= (int)layerSizes[ilayer-1];
		}
		inEnd -= layerSizes[ilayer-1];
		if (bias) con += inEnd * ((int)layerSizes[ilayer-1]+1);
		else con += inEnd * ((int)layerSizes[ilayer-1]+1) + inStart + 1;
		return con;
	}
	else if ((int)conRestrict == CR_NONE || conRestrict == CR_LAYERED) 
		return (inEnd - layerSizes[0]) * (neurons.size()+1) + inStart + 1;
	return 0;
}

// "neu" are the neurons stored from previous input in the same series
inline bool CPerceptronNN::next_connection (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu)
{
	static int wCount, inputCount;

	if (nEnd == NULL) {
		con = -1;
		wCount = weights->d.size();
		inputCount = layerSizes[0];
	}

	while (++con < wCount && weights->d[con] == 0);
	if (con == wCount) return false;

	nEnd = &neurons [conIndexes[con].end];
	if (neu != NULL && conIndexes[con].recurrent)
		nStart = &(*neu) [conIndexes[con].start - inputCount];
	else nStart = &neurons [conIndexes[con].start];
	return true;
}


inline bool CPerceptronNN::prev_connection (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu)
{
	static int inputCount;

	if (nEnd == NULL) {
		con = weights->d.size();
		inputCount = layerSizes[0];
	}

	while (--con >= 0 && weights->d[con] == 0);
	if (con == -1) return false;

	nEnd = &neurons [conIndexes[con].end];
	if (neu != NULL && conIndexes[con].recurrent)
		nStart = &(*neu) [conIndexes[con].start - inputCount];
	else nStart = &neurons [conIndexes[con].start];
	return true;
}

		
/*

inline bool CPerceptronNN::next_connection (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu)
{
	static int _conCount, inStart, inEnd;
	static int maxStart;
	static TVectorInt layerStarts;

	if (conRestrict == CR_NONE || conRestrict == CR_LAYERED) 
		return next_connection_recurrent (nStart, nEnd, con, neu);
	else if ((int)conRestrict == CR_LAYERED) {
		// initialize
		if (nEnd == NULL) {
			layerStarts.Realloc (layerSizes.size());
			_conCount = weights->d.size();
			con = 0;
			layerStarts[0] = 0;
			for (int ilayer=1; ilayer < layerSizes.size(); ++ilayer)
				layerStarts[ilayer] = layerStarts[ilayer-1] + layerSizes[ilayer-1];
			inStart = -1;
			inEnd = layerSizes[0];
			maxStart = layerStarts[neurons[inEnd].layer]-1;
			nStart = &neurons[-1];
			nEnd = &neurons[inEnd];
			return true;
		}

		if (++con == _conCount)
			return false;

		if (inStart == -1) 
			inStart = (int)layerStarts[neurons[inEnd].layer-1];
		else if (++inStart > maxStart) {
			inStart = -1;
			nEnd = &neurons[++inEnd];
			maxStart = (int)layerStarts[nEnd->layer]-1;
		}
		nStart = &neurons[inStart];
		return true;
	}

	return false;
}

inline bool CPerceptronNN::prev_connection (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu)
{
	static int conCount, inStart, inEnd;
	static int minStart;
	static TVectorInt layerStarts;
           
	if (conRestrict == CR_NONE || conRestrict == CR_LAYERED) 
		return prev_connection_recurrent (nStart, nEnd, con, neu);
	if ((int)conRestrict == CR_LAYERED) {
		// initialize
		if (nEnd == NULL) {
			layerStarts.Realloc (layerSizes.size());
			layerStarts[0] = 0;
			for (int ilayer=1; ilayer < layerSizes.size(); ++ilayer) 
				layerStarts[ilayer] = (int)layerStarts[ilayer-1] + (int)layerSizes[ilayer-1];
			conCount = weights->d.size();
			con = conCount-1;
			inEnd = neurons.size()-1;
			inStart = (int)layerStarts[neurons[inEnd].layer]-1;
			minStart = (int)layerStarts[neurons[inStart].layer];
			nStart = &neurons[inStart];
			nEnd = &neurons[inEnd];
			return true;
		}

		if (--con < 0)
			return false;

		if (inStart == -1) {
			nEnd = &neurons[--inEnd];
			inStart = (int)layerStarts[nEnd->layer]-1;
			minStart = (int)layerStarts[neurons[inStart].layer];
		}
		else if (--inStart < minStart) 
			inStart = -1; 
		nStart = &neurons[inStart];
		return true;
	}

	return false;
}


// "neu" are the neurons stored from previous input in the same series
inline bool CPerceptronNN::next_connection_recurrent (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu)
{
	static int inputCount, neuCount;

	if (nEnd == NULL) {
		con = -1;
		inputCount = layerSizes[0];
		neuCount = neurons.size();
	}

	while (++con < weights->d.size() && weights->d[con] == 0);
	if (con == weights->d.size()) return false;

	int inEnd = con / (neuCount+1) + inputCount;
	int inStart = con % (neuCount+1) - 1;

	nEnd = &neurons[inEnd];
	nStart = &neurons[inStart];	
	if (inStart != -1 && nStart->layer != nEnd->layer - 1 && neu != NULL) 
		nStart = &(*neu)[inStart - layerSizes[0]];
	return true;
}


inline bool CPerceptronNN::prev_connection_recurrent (TPerceptron *&nStart, TPerceptron *&nEnd, int &con, TVectorPerceptron *neu)
{
	static int inputCount, neuCount;

	if (nEnd == NULL) {
		con = weights->d.size();
		inputCount = layerSizes[0];
		neuCount = neurons.size();
	}

	while (--con >= 0 && weights->d[con] == 0);
	if (con == -1) return false;

	int inEnd = con / (neuCount+1) + inputCount;
	int inStart = con % (neuCount+1) - 1;

	nEnd = &neurons[inEnd];
	nStart = &neurons[inStart];	
	if (inStart != -1 && nStart->layer != nEnd->layer - 1 && neu != NULL) 
		nStart = &(*neu)[inStart - layerSizes[0]];
	return true;
}
*/
inline double TPerceptron::transfer () 
{ 
	switch((int)transferFn) {
	case TF_LINEAR: return output;
	case TF_LOGSIG: return 1.0 / (1.0 + exp (-output*lambda));
	case TF_TANSIG: return 2 / (1 + exp(-2*output)) - 1;
	default: assert (false); return 0;
	}
}

inline double TPerceptron::transferDeriv ()
{
	switch((int)transferFn) {
	case TF_LINEAR: return 1;
	case TF_LOGSIG: return lambda * output * (1 - output); 
	case TF_TANSIG: return 1 - sqr (output);
	default: assert (false); return 0;
	}
}

#endif


