/** @file xmltempl.cpp

  @brief Templates for XML input / output

  @author Jakub Adámek
  Last modified $Id: xmltempl.h,v 1.4 2002/04/22 15:43:23 jakubadamek Exp $
*/

#include "traindata.h"
#include "perceptron.h"

/**
	STRUCTURE_OPT (structure name, write condition)
*/

XMLIZE (TLearningGoal,learningGoal)
	CHILD (error)
	CHILD (time)
	CHILD (epochs)
	child (errorImprove,"minErrorImprovement")
	child (nErrorImprove,"badErrorImproveEpochs")
END_XMLIZE

XMLIZE (TPerceptronStructure::TParams,params)
	CHILD(learningRate)
	CHILD_OPT(epsilon)	
	CHILD_OPT(zeta)	
	CHILD_OPT(c)			
	CHILD_OPT(h)		
	CHILD_OPT(mi)
	CHILD_OPT(mi_i)
	CHILD_OPT(mi_d)
END_XMLIZE	

XMLIZE (TPerceptronStructure::TNeuronRange,neuronRange)
	CHILD(layer)
	CHILD_OPT(from)
	CHILD_OPT(to)
END_XMLIZE
	
XMLIZE (TPerceptronStructure::TRecurrentConnections,recurrentConnections)
	STRUCTURE (start)
	STRUCTURE (end)
END_XMLIZE

XMLIZE (TColumn::TWindow, window)
	CHILD (left)
	CHILD (right)
	CHILD_OPT (empty)
END_XMLIZE

XMLIZE (TColumn, column)
	CHILD_ENUM(use,CUs)
	RP( if (use != CU_NO) { )
		CHILD_ENUM (type,FTs)
		CHILD_ENUM (translate,FTRs)
		RP( if (translate == FTR_FLOAT) { )
			CHILD_OPT (minR)
			CHILD_OPT (maxR)
		RP( }
		else if (translate == FTR_LINEAR) { )
			CHILD_OPT (stdev)
			CHILD_OPT (avg)
		RP( } )
		STRUCTURE_OPT (window, window.left != 0 || window.right != 0)
		STRUCTURE_OPT (categories, type == FT_CATEGORY)
		R( CHILD_OPT (digits) )
		P( if (digits != -1) CHILD (digits); )
	RP( } )
END_XMLIZE

XMLIZE (CTrainingData::TRange, range)
	CHILD (from)
	CHILD (to)
	CHILD_ENUM (type, SetTypes)
	CHILD_OPT (random)
	CHILD_OPT (rest)
END_XMLIZE

// the rest of the functions is in read_all, print_all
XMLIZE (TPerceptronStructure, perceptronStructure)
	CHILD_ENUM (trainProcedure, TrainPs)
	CHILD_ENUM (weightInitProcedure, WIs)
	CONTAINER_OPT (allRecurrentConnections)
	child_enum (conRestrict, "connectionRestriction", CRs)
	CHILD_OPT (dumpWeights)
	CHILD (dumpWeightsDelimiter)
	CHILD_OPT (dumpAll)
	CHILD_OPT (dumpAllInterval)
	CHILD_OPT (logErrors)
	CHILD_OPT (runWeightsPerturbation)
	CHILD_OPT (debug)
	structure (par, "params")
END_XMLIZE

XMLIZE (TTrainingDataFile, file)
	CONTAINER_OPT (streams)
END_XMLIZE

XMLIZE (CTrainingData, trainingData)
	R( xml->AddDefaults(); )	

	CONTAINER (columns)
	container_opt (files, "trainingDataFiles")
	CONTAINER_OPT (ranges)
	CHILD_OPT (series)
	RP( if (series) )
		CHILD (seriesSeparator)
	CHILD_OPT (dumpCfg)
	CHILD_OPT (examineColumns)
	R( setColIndexes(); )
END_XMLIZE
/*
XMLIZE (TrainingProcess::TBatch, batch)
	structure (trDataStructure, "trainingData");
	CHILD (trials)
	CHILD (stopOnSuccess)
	CHILD_OPT (findSmallest)
	R( CString mytag = "perceptronStructure";
   	   if (!(*xml)[mytag].Exist()) return mytag+" required ";
		return structure.read_all (&(*xml)[mytag]); )
	P( retval->AddChild (*structure.print_all()); )
END_XMLIZE

XMLIZE (TrainingProcess, trainingProcess)
	R( xml->AddDefaults(); )

	CONTAINER (batches)
	structure (goal, "learningGoal")
	CHILD_OPT (logName)
	CHILD_OPT (dumpNN)
	CHILD_OPT (startBatch)
END_XMLIZE
*/
XMLIZE (TStream, stream)
	CHILD_ENUM (type, StreamTypes)
	CHILD_ENUM (encoding, SEs)
	RP( if (type == ST_REMOTE) )
		attr (value, "value")
	R( CHILD (delimiter)
		delimiter.replace ("tab","\t");
		delimiter.replace ("TAB","\t");)
	P( CString delim = delimiter;
		delim.replace ("\t","tab");
		child (delim, "delimiter");)
END_XMLIZE
