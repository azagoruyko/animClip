#pragma once

#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDependencyNode.h>

#include <set>
#include <vector>
#include <map>
#include <string>
#include <chrono>

using namespace std;

#define TO_MSTR(x) MString(to_string(x).c_str())

const vector<string> AnimCurveTypes{ "animCurveTA", "animCurveTL", "animCurveTT", "animCurveTU", "animCurveUA", "animCurveUL", "animCurveUT", "animCurveUU" };
const vector<string> TangentTypes{ "global", "fixed", "linear", "flat", "spline", "step", "slow", "fast", "clamped", "plateau", "stepnext", "auto" };

const map<string, MFnAnimCurve::AnimCurveType> AnimCurveTypesMap = {
	{"animCurveTA", MFnAnimCurve::AnimCurveType::kAnimCurveTA},
	{"animCurveTL", MFnAnimCurve::AnimCurveType::kAnimCurveTL},
	{"animCurveTT", MFnAnimCurve::AnimCurveType::kAnimCurveTT},
	{"animCurveTU", MFnAnimCurve::AnimCurveType::kAnimCurveTU},
	{"animCurveUA", MFnAnimCurve::AnimCurveType::kAnimCurveUA},
	{"animCurveUL", MFnAnimCurve::AnimCurveType::kAnimCurveUL},
	{"animCurveUT", MFnAnimCurve::AnimCurveType::kAnimCurveUT},
	{"animCurveUU", MFnAnimCurve::AnimCurveType::kAnimCurveUU} };

const map<string, MFnAnimCurve::TangentType> TangentTypesMap = {
	{"global", MFnAnimCurve::TangentType::kTangentGlobal},
	{"fixed", MFnAnimCurve::TangentType::kTangentFixed},
	{"linear", MFnAnimCurve::TangentType::kTangentLinear},
	{"flat", MFnAnimCurve::TangentType::kTangentFlat},
	{"spline", MFnAnimCurve::TangentType::kTangentSmooth},
	{"step", MFnAnimCurve::TangentType::kTangentStep},
	{"slow", MFnAnimCurve::TangentType::kTangentSlow},
	{"fast", MFnAnimCurve::TangentType::kTangentFast},
	{"clamped", MFnAnimCurve::TangentType::kTangentClamped},
	{"plateau", MFnAnimCurve::TangentType::kTangentPlateau},
	{"stepnext", MFnAnimCurve::TangentType::kTangentStepNext},
	{"auto", MFnAnimCurve::TangentType::kTangentAuto},
};

inline chrono::steady_clock::time_point getMeasureTime() { return chrono::steady_clock::now(); }

inline void measureTime(const string& name, const chrono::steady_clock::time_point& startTime)
{
	const auto count = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - startTime).count();
	printf("%s: %.2fs", name.c_str(), count / 1000000.0);
}

inline string getNodeLocalName(const MFnDependencyNode &nodeFn)
{
	MStringArray nameParts;
	nodeFn.name().split(':', nameParts);
	return string(nameParts[nameParts.length() - 1].asChar());
}

inline bool endsWith(const string &str, const string &suffix)
{
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

inline bool startsWith(const string &str, const string &prefix)
{
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

inline string replaceString(string subject, const string &search, const string &replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

inline void findAnimationCurves(const MPlug& plug, MObjectArray& outAnimCurves)
{
	MObjectArray animCurves;
	set<const char*> skip;

	if (MAnimUtil::findAnimation(plug, animCurves))
	{
		for (int k = 0; k < animCurves.length(); k++)
		{
			MFnAnimCurve acFn(animCurves[k]);
			MPlugArray destPlugs;
			if (acFn.findPlug("output", true).destinationsWithConversions(destPlugs))
			{
				for (int j = 0; j < destPlugs.length(); j++)
				{
					if (destPlugs[j].node() == plug.node() && skip.find(acFn.name().asChar()) == skip.end())
					{
						outAnimCurves.append(animCurves[k]);
						skip.emplace(acFn.name().asChar());
					}
				}
			}
		}
	}
}