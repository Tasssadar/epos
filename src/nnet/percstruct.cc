/** @file percstruct.cpp

  @brief Perceptron Structure XML in / out

  @author Jakub Adámek
  Last modified $Id: percstruct.cc,v 1.3 2002/04/22 15:43:23 jakubadamek Exp $
*/

#include "perceptron.h"
//#include "trainprocess.h"
#include "utils.h"
#include "xmlutils.h"
#include "xml.h"

REGSTRUCT (TPerceptronStructure)
REGSTRUCTC (TPerceptronStructure, TParams)
REGSTRUCTC (TPerceptronStructure, TRecurrentConnections)
REGSTRUCTC (TPerceptronStructure, TNeuronRange)
REGVECTORC (TPerceptronStructure, TRecurrentConnections, TAllRecurrentConnections)

CXml * TPerceptronStructure::print_all () const
{
	CXml * retVal = print ();

	CString lsizes, trfuncs;
	lsizes = join (layerSizes.begin(), layerSizes.end(), "-");
	for (int i=1; i < layerTrFuncs.size(); ++i) {
		if (i > 1) trfuncs += "-";
		trfuncs += TFns[layerTrFuncs[i]];
	}

	retVal->AddChild (*xml_print (lsizes, "layerSizes"));
	retVal->AddChild (*xml_print (trfuncs,"layerTransferFuncs"));
	return retVal;
}

CXml * CPerceptronNN::print_all (const CString &filename, CTrainingData *trData) const
{
	CXml *retVal = TPerceptronStructure::print_all();

	retVal->AddChild (
		*(new CXml ("weights",0,1,printWeights (filename)))->SetFF(DODELETE));
	if (trData) 
		retVal->AddChild (*trData->print());

	return retVal;
}

CString TPerceptronStructure::read_all (CRox *xml)
{
	int i;
	CString value;
	CString err;
	CString die = "PerceptronStructure::read failed - ";

	err = read (xml);
	err += xml_read (xml, value, "layerSizes");

	if (err.length()) return die+err;

	TVectorString l = split (value, "-");
	layerSizes.Realloc (l.size());
	for (i=0; i < l.size(); ++i)
		layerSizes[i] = atoi (l[i]);

	err += xml_read (xml, value, "layerTransferFuncs");
	if (err.length()) return die+err;
	l = split (value, "-");
	if (l.size() != layerSizes.size()-1)
		return die+"Layer transfer func count must be 1 less than layer sizes count";
	layerTrFuncs.Realloc (l.size()+1);
	for (i=0; i < l.size(); ++i) {
		layerTrFuncs[i+1] = TFns[l[i]];
		if (layerTrFuncs[i+1] == -1)
			return die+"Wrong transfer function " + l[i];
	}

	if (err.length()) err = die+err;
	return err;
}

CString CPerceptronNN::read_all (CRox *xml, const CString &filename, CTrainingData *trData)
{
	if (xml == 0) return "Error: empty XML - perhaps a result of wrong parsing.";
	CString err = TPerceptronStructure::read_all (xml);
	if (err.length()) return err;

	CString die = "PerceptronNN::read failed - ";
	initNeurons();

	CRox *child = &(*xml)["weights"];
	if (child->Exists()) {
		child = &(*child)["stream"];
		if (child->Exists()) {
			initConAttribs ();
			err = readWeights (*child, filename);
			if (err.length()) return die+err;
		}
		else err += "Weights stream missing";
	}

	if (trData != NULL) {
		child = &(*xml)["trainingData"];
		if (!child->Exists()) err += "Training data requested but not included in the PerceptronNN config file";
		else err += trData->read (child);
	}

	return err;
}

