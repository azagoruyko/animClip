#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MSyntax.h>
#include <maya/MDGModifier.h>
#include <maya/MAnimCurveChange.h>
#include <maya/MSelectionList.h>

class LoadAnimClipCommand : public MPxCommand
{
public:
	static void* creator() { return new LoadAnimClipCommand(); }

	static MSyntax newSyntax();

	virtual bool isUndoable() const { return true; }

	virtual MStatus doIt(const MArgList& args);
	virtual MStatus undoIt();
	virtual MStatus redoIt();

private:
	MDGModifier m_dgmod;
	MAnimCurveChange m_animChange;
	MSelectionList m_objectList;
	MString m_namespace;
	
	MString m_filePath;
	double m_startFrame;
};