/** @file trainprocess.h

  @brief CTrainingProcess header

  @author Jakub Adámek
  Last modified $Id: trainprocess.h,v 1.13 2002/04/22 15:43:23 jakubadamek Exp $
*/

#ifndef __BANG_TRAINPROCESS_H__
#define __BANG_TRAINPROCESS_H__

#include "math.h"
#include "time.h"
#include "traindata.h"
#include "perceptron.h"
#include "../basiciterator.h"
#include "../../base/bang.h"

enum TNNFitness {
	/// fitness over different initial weights
	NF_WEIGHTS
};
static const TEnumString NFs = "weights";

/** @brief Network properties related to training process and managing
		of files containing network image.

	In the simpliest case only runs one training batch given number of times and finds
	the best result.

  	You can handle the sets - train one set first, than another. This way you can
	add information to a trained network not destroying the old knowledge.
*/

AGENT_BEGIN_FROM( TrainingProcess, basiciterator )
public:
	void Init ();	

	/// calculates fitness for given parameter
	CFloat fitness (TNNFitness which);

	STRUCT_BEGIN (TBatch)
	public:
		void Init()	{ trials=1; stopOnSuccess=true; findSmallest=false; }
		TPerceptronStructure structure;
		CTrainingData trDataStructure;
		/** how many times to try to achieve the error goal - on achieving it 
			the process goes to next batch */
		CInt trials;
		/** if error achieved, should I stop and go to next batch?
			usually "true" is good for practical purposes, "false" for algorithm evaluation */
		CInt stopOnSuccess;
		/** try different layer sizes to find smallest network - multiply the layer sizes
			by findSmallest each time or add 1 if this is not enough*/
		CFloat findSmallest;
	STRUCT_END (TBatch)
	VECTORC (TBatch, TBatches)

	TBatches batches;
	TLearningGoal goal;

	/// log of the training process
	CString logName;
	/// dump of the network structure 
	CString dumpNN;

	/// you can jump over some batches by startBatch 
	CInt startBatch;

	void run (CTrainingData *trainingData, const CString &filename);

//protected:
	/// data used to train the network
	//CWeakPtr trData;
	CTrainingData *trData;

	virtual CString start ();
	virtual int stopCriterion ();
	virtual CString nextIteration ();
	virtual CString end ();

	CXml *print () const; 
	CString read (CRox *xml);
private:
	CAgent TrainingDataAgent;
	CAgent PerceptronNNAgent;

	/// local variables used in iterations
	TLearningGoal goalState;
	CString goalString;
	CString netFileName, trdataFileName;
	/// configuration file of this training process
	CString filename;
	CString logs, trfuncs;
	CInt batchIndex;
	TBatches::iterator ibatch;
	CInt cycle;
	enum {CY_BATCH, CY_END, CY_SMALLEST, CY_SMALLEST_END, CY_TRIAL};
	CFloat minError, lastMinError;
	CInt itrial;
AGENT_END_FROM( TrainingProcess, basiciterator )

#endif

