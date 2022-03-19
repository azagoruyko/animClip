#include <maya/MGlobal.h>
#include <maya/MDoubleArray.h>
#include <maya/MSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <maya/MAnimUtil.h>
#include <maya/MAnimControl.h>
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
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/writer.h"

#include "utils.h"

#include "loadAnimClipCommand.h"

using namespace std;
using namespace rapidjson;

MSyntax LoadAnimClipCommand::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag("-f", "-file", MSyntax::MArgType::kString);
	syntax.addFlag("-sf", "-startFrame", MSyntax::MArgType::kLong);

	return syntax;
};

MStatus LoadAnimClipCommand::doIt(const MArgList& args)
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

	redoIt();
	return MS::kSuccess;
}

MObject getMObjectByName(const MString& name)
{
	MSelectionList selList;
	if (MGlobal::getSelectionListByName(name, selList) != MS::kSuccess || selList.length() == 0)
		return MObject();

	MObject object;
	selList.getDependNode(0, object);
	return object;
}

void setAnimCurveData(MFnAnimCurve& acFn, const Value& animData, MAnimCurveChange *animChange, double timeOffset = 0)
{
	const double coeff = acFn.animCurveType() == MFnAnimCurve::AnimCurveType::kAnimCurveTA || // degrees to radians
						 acFn.animCurveType() == MFnAnimCurve::AnimCurveType::kAnimCurveUA ? 0.0174532862 : 1;

	const bool isWeighted = animData["weighted"].GetBool();
	acFn.setIsWeighted(isWeighted);

	const auto unit = (MTime::Unit)animData["unit"].GetInt();

	for (const auto& frameData : animData["data"].GetArray())
	{
		const auto &fdata = frameData.GetArray();
		const double t(fdata[0].GetDouble());
		const double v(fdata[1].GetDouble());
		const auto itt = TangentTypesMap.at(fdata[2].GetString());
		const auto ott = TangentTypesMap.at(fdata[3].GetString());

		const double time = t + timeOffset;
		const double value = v * coeff;

		int idx;
		if (acFn.isTimeInput())
			idx = acFn.addKey(MTime(time, unit), value, MFnAnimCurve::TangentType::kTangentGlobal, MFnAnimCurve::TangentType::kTangentGlobal, animChange);
		else
			idx = acFn.addKey(time, value, MFnAnimCurve::TangentType::kTangentGlobal, MFnAnimCurve::TangentType::kTangentGlobal, animChange);
		
		if (itt == MFnAnimCurve::kTangentFixed || ott == MFnAnimCurve::kTangentFixed)
		{
			const bool wl = fdata[4].GetBool();
			const bool tl = fdata[5].GetBool();
			const MAngle ia(fdata[6].GetDouble());
			const MAngle oa(fdata[7].GetDouble());
			const double iw(fdata[8].GetDouble());
			const double ow(fdata[9].GetDouble());

			acFn.setWeightsLocked(idx, false);
			acFn.setTangentsLocked(idx, false);
			acFn.setTangent(idx, ia, iw, true);
			acFn.setTangent(idx, oa, ow, false);

			if (isWeighted)
			{
				const double ix(fdata[10].GetDouble());
				const double iy(fdata[11].GetDouble());
				const double ox(fdata[12].GetDouble());
				const double oy(fdata[13].GetDouble());
				acFn.setTangent(idx, ix, iy, true);
				acFn.setTangent(idx, ox, oy, false);
			}

			acFn.setWeightsLocked(idx, wl);
			acFn.setTangentsLocked(idx, tl);
		}

		acFn.setInTangentType(idx, itt);
		acFn.setOutTangentType(idx, ott);
	}
}

MStatus LoadAnimClipCommand::redoIt()
{
	const double currentFrame = m_startFrame == DBL_MAX ? MAnimControl::currentTime().value() : m_startFrame;

	ifstream ifs(m_filePath.asChar());
	IStreamWrapper isw(ifs);

	Document doc;
	doc.ParseStream(isw);

	MSelectionList selList;
	MGlobal::getActiveSelectionList(selList);

	for (int i = 0; i < selList.length(); i++)
	{
		MObject nodeObj;
		selList.getDependNode(i, nodeObj);

		MFnDependencyNode nodeFn(nodeObj);
		string nodeLocalName = getNodeLocalName(nodeFn);

		if (!doc.HasMember(nodeLocalName.c_str()))
		{
			string mirrorName;
			if (startsWith(nodeLocalName, "L_"))
				mirrorName = "R_"+ nodeLocalName.substr(2);

			else if (startsWith(nodeLocalName, "R_"))
				mirrorName = "L_" + nodeLocalName.substr(2);

			else if (startsWith(nodeLocalName, "l_"))
				mirrorName = "r_" + nodeLocalName.substr(2);

			else if (startsWith(nodeLocalName, "r_"))
				mirrorName = "l_" + nodeLocalName.substr(2);

			else if (endsWith(nodeLocalName, "_L"))
				mirrorName = nodeLocalName.substr(0, nodeLocalName.length()-2) + "_R";

			else if (endsWith(nodeLocalName, "_R"))
				mirrorName = nodeLocalName.substr(0, nodeLocalName.length() - 2) + "_L";

			else if (endsWith(nodeLocalName, "_l"))
				mirrorName = nodeLocalName.substr(0, nodeLocalName.length() - 2) + "_r";

			else if (endsWith(nodeLocalName, "_r"))
				mirrorName = nodeLocalName.substr(0, nodeLocalName.length() - 2) + "_l";

			if (!doc.HasMember(mirrorName.c_str()))
			{
				MGlobal::displayWarning("Cannot find '" + MString(nodeLocalName.c_str()) + "' in clip");
				continue;
			}

			nodeLocalName = mirrorName;
		}

		const char *node = nodeLocalName.c_str();

		for (const auto &data : doc[node]["animation"].GetObject()) // per every animation curve data
		{
			const MString attrName(data.name.GetString());

			MPlug destPlug = nodeFn.findPlug(attrName, true);
			if (destPlug.isNull())
			{
				MGlobal::displayWarning("Cannot find '" + nodeFn.name() + "." + attrName + "'");
				continue;
			}

			if (!destPlug.isLocked())
			{
				MObjectArray animCurves;
				findAnimationCurves(destPlug, animCurves);

				if (animCurves.length() == 0)
				{
					MFnAnimCurve acFn;
					MObject ac = acFn.create(nodeObj, destPlug.attribute(), &m_dgmod);
					m_dgmod.renameNode(ac, MString(node) + "_"+ attrName);

					acFn.setPreInfinityType((MFnAnimCurve::InfinityType)data.value["preinf"].GetInt());
					acFn.setPostInfinityType((MFnAnimCurve::InfinityType)data.value["postinf"].GetInt());

					setAnimCurveData(acFn, data.value, NULL, currentFrame);
				}
				else
				{
					for (int k = 0; k < animCurves.length(); k++)
						setAnimCurveData(MFnAnimCurve(animCurves[k]), data.value, &m_animChange, currentFrame);
				}
			}
		}

		for (const auto &attrData : doc[node]["static"].GetObject()) // per every static attribute
		{
			const MString attrName(attrData.name.GetString());
			MPlug destPlug = nodeFn.findPlug(attrName, true);
			if (destPlug.isNull())
			{
				MGlobal::displayWarning("Cannot find '" + nodeFn.name() + "." + attrName + "'");
				continue;
			}

			if (!destPlug.isLocked())
				m_dgmod.newPlugValueDouble(destPlug, attrData.value.GetDouble());
		}
	}

	m_dgmod.doIt();
	m_animChange.redoIt();

	MGlobal::displayInfo("Import anim clip from '" + m_filePath + "'");

	return MS::kSuccess;
}

MStatus LoadAnimClipCommand::undoIt()
{
	m_animChange.undoIt();
	m_dgmod.undoIt();

	return MS::kSuccess;
}