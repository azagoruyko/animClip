#include <maya/MFnPlugin.h>

#include "saveAnimClipCommand.h"
#include "loadAnimClipCommand.h"

MStatus initializePlugin(MObject plugin)
{
	MFnPlugin pluginFn(plugin);
	pluginFn.registerCommand("saveAnimClip", SaveAnimClipCommand::creator, SaveAnimClipCommand::newSyntax);
	pluginFn.registerCommand("loadAnimClip", LoadAnimClipCommand::creator, LoadAnimClipCommand::newSyntax);
	return MS::kSuccess;
}

MStatus uninitializePlugin(MObject plugin)
{
	MFnPlugin pluginFn(plugin);
	pluginFn.deregisterCommand("saveAnimClip");
	pluginFn.deregisterCommand("loadAnimClip");
	return MS::kSuccess;
}