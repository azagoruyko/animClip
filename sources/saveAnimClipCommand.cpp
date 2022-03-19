#include <maya/MGlobal.h>
#include <maya/MDoubleArray.h>
#include <maya/MSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>

#include <vector>
#include <set>
#include <string>
#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"

#include "utils.h"

#include "saveAnimClipCommand.h"

using namespace std;
using namespace rapidjson;

MSyntax SaveAnimClipCommand::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag("-f", "-file", MSyntax::MArgType::kString);
	syntax.addFlag("-sf", "-startFrame", MSyntax::MArgType::kLong);
	syntax.addFlag("-ef", "-endFrame", MSyntax::MArgType::kLong);

	return syntax;
};

void getAnimCurveFrameData(const MFnAnimCurve& acFn, Value &outArray, Document::AllocatorType& allocator, double startFrame = DBL_MAX, double endFrame = DBL_MAX)
{
	// radians to degrees coeff
	const double coeff = acFn.animCurveType() == MFnAnimCurve::kAnimCurveTA || acFn.animCurveType() == MFnAnimCurve::kAnimCurveUA ? 57.2958 : 1;

	for (int i = 0; i < acFn.numKeys(); i++)
	{
		double t = acFn.isUnitlessInput() ? acFn.unitlessInput(i) : acFn.time(i).value();
		if (endFrame != DBL_MAX && t > endFrame)
			continue;

		if (startFrame != DBL_MAX)
		{
			if (t >= startFrame)
				t -= startFrame;
			else
				continue;
		}

		const auto itt = acFn.inTangentType(i);
		const auto ott = acFn.inTangentType(i);

		Value item(kArrayType);
		item.PushBack(t, allocator);
		item.PushBack(acFn.value(i) * coeff, allocator);
		item.PushBack(Value(TangentTypes[itt].c_str(), allocator).Move(), allocator);
		item.PushBack(Value(TangentTypes[ott].c_str(), allocator).Move(), allocator);

		if (itt == MFnAnimCurve::kTangentFixed || ott == MFnAnimCurve::kTangentFixed)
		{
			MAngle ia, oa;
			double iw = 0, ow = 0;

			acFn.getTangent(i, ia, iw, true);
			acFn.getTangent(i, oa, ow, false);

			item.PushBack(acFn.weightsLocked(i), allocator);
			item.PushBack(acFn.tangentsLocked(i), allocator);
			item.PushBack(ia.value(), allocator);
			item.PushBack(oa.value(), allocator);
			item.PushBack(iw, allocator);
			item.PushBack(ow, allocator);

			if (acFn.isWeighted())
			{
				MFnAnimCurve::TangentValue ix, ox;
				MFnAnimCurve::TangentValue iy, oy;

				acFn.getTangent(i, ix, iy, true);
				acFn.getTangent(i, ox, oy, false);

				item.PushBack(ix, allocator);
				item.PushBack(iy, allocator);
				item.PushBack(ox, allocator);
				item.PushBack(oy, allocator);
			}
		}

		outArray.PushBack(item, allocator);
	}
}

void getAnimCurveData(const MObject &animCurveObject, Value &outObject, Document::AllocatorType& allocator, double startFrame = DBL_MAX, double endFrame = DBL_MAX)
{
	MFnAnimCurve acFn(animCurveObject);
	MPlug outputPlug = acFn.findPlug("output", true);

	MPlugArray destinations;
	MString nodeName, attrName;
	if (outputPlug.destinationsWithConversions(destinations))
	{
		MFnDependencyNode nodeFn(destinations[0].node());
		MStringArray nameParts;
		nodeFn.name().split(':', nameParts);
		nodeName = nameParts[nameParts.length() - 1].asChar();
		attrName = destinations[0].partialName().asChar();
	}

	outObject.AddMember("weighted", acFn.isWeighted(), allocator);
	outObject.AddMember("preinf", acFn.preInfinityType(), allocator);
	outObject.AddMember("postinf", acFn.postInfinityType(), allocator);
	outObject.AddMember("unit", (int)acFn.numKeys() > 0 ? acFn.time(0).unit() : 0, allocator);
	outObject.AddMember("data", Value(kArrayType), allocator);

	getAnimCurveFrameData(acFn, outObject["data"], allocator, startFrame, endFrame);
}

MStatus SaveAnimClipCommand::doIt(const MArgList& args)
{
	MArgParser argParser(syntax(), args);

	if (!argParser.isFlagSet("-f"))
	{
		MGlobal::displayError("-file(-f) flag must be specified with a file path");
		return MS::kFailure;
	}

	argParser.getFlagArgument("-f", 0, m_filePath);

	if (argParser.isFlagSet("-sf"))
		argParser.getFlagArgument("-sf", 0, m_startFrame);
	else
		m_startFrame = DBL_MAX;

	if (argParser.isFlagSet("-ef"))
		argParser.getFlagArgument("-ef", 0, m_endFrame);
	else
		m_endFrame = DBL_MAX;

	redoIt();
	return MS::kSuccess;
}

MStatus SaveAnimClipCommand::redoIt()
{
	MDoubleArray result;
	MGlobal::executeCommand("timeControl -q -ra $gPlayBackSlider;", result);
	const double startFrame = m_startFrame == DBL_MAX ? result[0] : m_startFrame;
	const double endFrame = m_endFrame == DBL_MAX ? result[1] : m_endFrame;

	MSelectionList selList;
	MGlobal::getActiveSelectionList(selList);

	Document doc(kObjectType);

	if (endFrame - startFrame <= 1.0)
	{
		// save current pose for selected nodes
		for (int i = 0; i < selList.length(); i++)
		{
			MObject nodeObj;
			selList.getDependNode(i, nodeObj);
			MFnDependencyNode nodeFn(nodeObj);

			const string nodeName = getNodeLocalName(nodeFn);
			const char *node = nodeName.c_str();


			if (!doc.HasMember(node))
			{
				doc.AddMember(Value(node, doc.GetAllocator()).Move(), Value(kObjectType), doc.GetAllocator());

				doc[node].AddMember("animation", Value(kObjectType), doc.GetAllocator());
				doc[node].AddMember("static", Value(kObjectType), doc.GetAllocator());
				doc[node].AddMember("others", Value(kObjectType), doc.GetAllocator());
			}

			for (int k = 0; k < nodeFn.attributeCount(); k++)
			{
				const MPlug plug(nodeObj, nodeFn.attribute(k));
				if (plug.isKeyable())
					doc[node]["static"].AddMember(Value(plug.partialName().asChar(), doc.GetAllocator()).Move(), Value(plug.asDouble()), doc.GetAllocator());
			}

			// save rotateOrder for each selected node
			const MPlug p = nodeFn.findPlug("ro", true);
			if (!p.isNull())
				doc[node]["static"].AddMember(Value("ro", doc.GetAllocator()).Move(), Value(p.asShort()), doc.GetAllocator());
		}

		MGlobal::displayInfo("Export pose clip to '" + m_filePath + "'");
	}
	else
	{
		// find animation curves on selected objects and export them
		MPlugArray plugs;
		MAnimUtil::findAnimatedPlugs(selList, plugs, false);

		for (int i = 0; i < plugs.length(); i++)
		{
			MObjectArray animCurves;
			findAnimationCurves(plugs[i], animCurves);

			if (animCurves.length() > 0)
			{
				MFnDependencyNode nodeFn(plugs[i].node());
				const string nodeName = getNodeLocalName(nodeFn);
				const string attrName(plugs[i].partialName().asChar());

				const char* node = nodeName.c_str();
				const char* attr = attrName.c_str();

				if (!doc.HasMember(node))
				{
					doc.AddMember(Value(node, doc.GetAllocator()).Move(), Value(kObjectType), doc.GetAllocator());

					doc[node].AddMember("animation", Value(kObjectType), doc.GetAllocator());
					doc[node].AddMember("static", Value(kObjectType), doc.GetAllocator());
					doc[node].AddMember("others", Value(kObjectType), doc.GetAllocator());
				}

				// save rotateOrder for each selected node
				if (!doc[node]["static"].HasMember("ro"))
				{
					const MPlug p = nodeFn.findPlug("ro", true);
					if (!p.isNull())
						doc[node]["static"].AddMember(Value("ro", doc.GetAllocator()).Move(), Value(p.asShort()), doc.GetAllocator());
				}

				doc[node]["animation"].AddMember(Value(attr, doc.GetAllocator()).Move(), Value(kObjectType), doc.GetAllocator());

				getAnimCurveData(animCurves[0], doc[node]["animation"][attr], doc.GetAllocator(), startFrame, endFrame);
			}
		}

		MGlobal::displayInfo("Export anim clip in range " + TO_MSTR(int(startFrame)) + ".." + TO_MSTR(int(endFrame)) + " to '" + m_filePath+"'");
	}

	ofstream ofs(m_filePath.asChar());
	OStreamWrapper osw(ofs);

	Writer<OStreamWrapper> writer(osw);
	doc.Accept(writer);

	return MS::kSuccess;
}

MStatus SaveAnimClipCommand::undoIt()
{
	return MS::kSuccess;
}