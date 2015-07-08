/** @file percstruct.h

  @brief TPerceptronStructure header 

  @author Jakub Adámek
  Last modified $Id: percstruct.h,v 1.5 2002/04/23 15:49:41 jakubadamek Exp $
*/

#ifndef __BANG_PERCSTRUCT_H__
#define __BANG_PERCSTRUCT_H__

#include "math.h"
#include "time.h"
//#include "../../base/bing.h"

/// abstract neuron transfer function 
typedef double TFloat2FloatFn (double,double);

#undef LABELEASYC
#define LABELEASYC(x,y) x y

/** @brief Represents the learning goal on which training process stops. */
STRUCT_BEGIN (TLearningGoal)
	void Init ()	{ error=0.01; evalError=0; time=600; epochs=1000; errorImprove=0.00001;
			  nErrorImprove=30; }
	enum typeEnum { GT_NOTMET, GT_ERROR, GT_TIME, GT_EPOCHS, GT_ERROR_IMPROVEMENT, GT_EVAL_IMPROVEMENT };
	/// which goal was met - typeEnum
	LABELEASYC (CInt, goalMet);
	/// desired result: error not greater than this
	LABELEASYC (CFloat, error);
	/// information only - error on evaluation set
	LABELEASYC (CFloat, evalError);
	/// max time in seconds
	LABELEASYC (CInt, time);
	/// max number of epochs
	LABELEASYC (CInt, epochs);
	/** if the error (or evalError, if there is eval set) 
		doesn't improve enough for a number of epochs, stop your job */
	LABELEASYC (CFloat, errorImprove);
	/// if the error doesn't improve enough for a number of epochs, stop your job
	LABELEASYC (CInt, nErrorImprove);
//	bool operator < (const TLearningGoal &y) const { return true; }
STRUCT_END (TLearningGoal)

static const TEnumString goalMetString = "Nothing;Hurrah!;Time limit crossed;Too many epochs;"
	"Error doesn't improve for too long;Generalization optimum on evaluation set reached";

class CPerceptronNN;

/// forward connections restriction (no restrictions exist for recurrent connections)
enum conRestrictEnum {
	/// connections are only between neurons layer i -> layer i+1 
	CR_LAYERED,
	/// any connections between neurons i -> j, i < j
	CR_NONE
};

static const TEnumString CRs = "layered;none";

enum TTrainProcedures {
	// simply lowers weights by a part of negative gradient 
	TP_GRADIENT_DESCENT,
	// a special powerful algorithm by Rueger 
	TP_STABLE_CONJUGATE,
	// slightly improved quazi-Newton Levenberg-Marquardt method 
	TP_MLM,
	TP_LM,
	// no learning: only reads the weights and allows to run the network
	TP_RUN_ONLY
};

static const TEnumString TrainPs = "gradientDescent;stableConjugateGradient;modifiedLM;LM;runOnly";

enum TWeightInit {
	// random initialization in <-n^.5;n^.5>, n is count of connections coming to the end neuron 
	WI_RANDOM,
	WI_NGUYEN_WIDROW
};

static const TEnumString WIs = "random;nguyen-widrow";

enum TTransferFn {
	TF_LOGSIG,
	TF_LINEAR,
	TF_TANSIG
};

static const TEnumString TFns = "logsig;linear;tansig";

/** @brief Structure of a perceptron network
	
	  Contains all info determining network structure. Used both by
	  CTrainingProcess and CPerceptron
*/
STRUCT_BEGIN (TPerceptronStructure)
	void Init()	{ conRestrict=CR_LAYERED; weightInitProcedure=WI_RANDOM;
			  trainProcedure=TP_GRADIENT_DESCENT; debug=0; }

	/// see conRestrictEnum
	CInt conRestrict;
	/// see TWeightInit
	CInt weightInitProcedure;
	/// see TTrainProcedures
	CInt trainProcedure;

	/** @brief Parametres used in various methods */
	STRUCT_BEGIN (TParams)
		void Init ()	{ learningRate=0.01; momentum=0.2; epsilon=0.00001; zeta=1.5; c=0.5; 
				  h=0.005; mi=0.001; mi_i=2; mi_d=0.6; }
		CFloat learningRate;
		CFloat momentum;
		/// node perturbation: the mini shift for derivation calculation
		CFloat epsilon;
		/// stable conjugate gradient: learning rate multiplicator
		CFloat zeta; 
		/// stable conjugate gradient: 
		CFloat c;
		/// modified L-M:
		CFloat h;
		/// modified L-M: the main parameter
		CFloat mi;
		/// modified L-M: increase in mi
		CFloat mi_i;
		/// modified L-M: decrease in mi
		CFloat mi_d;
		/** Minimal change to be used when adapting weights. 
			I am not really sure it improves learning, but you can set it to 0 at any time 
			to avoid use of it. 
		    Not yet implemented. */
		CFloat minErrorChange;
	STRUCT_END (TParams)

	/// parametres used in various methods
	TParams par;
	/// learning goal 
	TLearningGoal goal;

	/// layerSizes[0] = input count, layerSizes[layerSizes.size()-1] = output count
	TVectorInt layerSizes;
	/// layer transfer functions .. layerTrFuncs[0] ignored - doesn't have sence for input layer
	TVectorInt layerTrFuncs;

	STRUCT_BEGIN (TNeuronRange)
		void Init()	{ layer=0; from=0; to=-1; }
		CInt from, to, layer;
	STRUCT_END (TNeuronRange);

	/** specifies a range of connections - neurons in layer[0] with indexes from[0]..to[0]
		have connections to neurons in layer[1] with indexes from[1]..to[1] */
	STRUCT_BEGIN (TRecurrentConnections)
		TNeuronRange start, end;
	STRUCT_END (TRecurrentConnections)
	VECTORC (TRecurrentConnections, TAllRecurrentConnections)

	TAllRecurrentConnections allRecurrentConnections;

	/// log of training and evaluation errors
	CString logErrors;
	/// file name for final weights dump
	CString dumpWeights;
	CString dumpWeightsDelimiter;
	/// where to dump network (backup)
	CString dumpAll;
	/// interval between backups
	CInt dumpAllInterval;

	/// if true, runs the weights perturbation after each backpropagation to prove the results are OK
	CInt runWeightsPerturbation;

	/// debug level - the higher the more messages are displayed
	CInt debug;

	/// prints the structure
	virtual CXml *print_all () const;
	/** Reads the structure. Returns error description or empty string. 
		The filename of the config file is used to find the path to the weights stream */
	virtual CString read_all (CRox *xml);
STRUCT_END (TPerceptronStructure)

#endif

