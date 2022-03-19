#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MSyntax.h>

class SaveAnimClipCommand : public MPxCommand
{
public:
	static void* creator() { return new SaveAnimClipCommand(); }

	static MSyntax newSyntax();

	virtual bool isUndoable() const { return false; }

	virtual MStatus doIt(const MArgList& args);
	virtual MStatus undoIt();
	virtual MStatus redoIt();

private:
	MString m_filePath;

	double m_startFrame;
	double m_endFrame;
};