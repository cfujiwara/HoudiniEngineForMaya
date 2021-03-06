#include "InputGeometryNode.h"

#include <maya/MFnGenericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNumericAttribute.h>

#include "MayaTypeID.h"
#include "Input.h"

MString InputGeometryNode::typeName("houdiniInputGeometry");
MTypeId InputGeometryNode::typeId(MayaTypeID_HoudiniInputGeometryNode);

MObject InputGeometryNode::inputTransform;
MObject InputGeometryNode::inputGeometry;
MObject InputGeometryNode::outputNodeId;

void*
InputGeometryNode::creator()
{
    return new InputGeometryNode();
}

MStatus
InputGeometryNode::initialize()
{
    MFnGenericAttribute gAttr;
    MFnMatrixAttribute mAttr;
    MFnNumericAttribute nAttr;

    InputGeometryNode::inputTransform = mAttr.create(
            "inputTransform", "inputTransform"
            );
    addAttribute(InputGeometryNode::inputTransform);

    InputGeometryNode::inputGeometry = gAttr.create(
            "inputGeometry", "inputGeometry"
            );
    gAttr.addDataAccept(MFnData::kIntArray);
    gAttr.addDataAccept(MFnData::kMesh);
    gAttr.addDataAccept(MFnData::kNurbsCurve);
    gAttr.addDataAccept(MFnData::kVectorArray);
    gAttr.setCached(false);
    gAttr.setStorable(false);
    addAttribute(InputGeometryNode::inputGeometry);

    InputGeometryNode::outputNodeId = nAttr.create(
            "outputNodeId", "outputNodeId",
            MFnNumericData::kInt,
            -1
            );
    nAttr.setCached(false);
    nAttr.setStorable(false);
    addAttribute(InputGeometryNode::outputNodeId);

    attributeAffects(InputGeometryNode::inputTransform, InputGeometryNode::outputNodeId);
    attributeAffects(InputGeometryNode::inputGeometry, InputGeometryNode::outputNodeId);

    return MStatus::kSuccess;
}

MStatus
InputGeometryNode::compute(
        const MPlug &plug,
        MDataBlock &dataBlock
        )
{
    if(plug == InputGeometryNode::outputNodeId)
    {
        MDataHandle outputNodeIdHandle = dataBlock.outputValue(InputGeometryNode::outputNodeId);

        if(!checkInput(dataBlock))
        {
            outputNodeIdHandle.setInt(-1);

            return MStatus::kFailure;
        }

        // set input transform
        MPlug transformPlug(thisMObject(), InputGeometryNode::inputTransform);
        MDataHandle transformHandle = dataBlock.inputValue(transformPlug);
        myInput->setInputTransform(transformHandle);

        // set input geo
        MPlug geometryPlug(thisMObject(), InputGeometryNode::inputGeometry);
        myInput->setInputGeo(dataBlock, geometryPlug);

        outputNodeIdHandle.setInt(myInput->geometryNodeId());

        return MStatus::kSuccess;
    }

    return MPxNode::compute(plug, dataBlock);
}

InputGeometryNode::InputGeometryNode() :
    myInput(NULL)
{
}

InputGeometryNode::~InputGeometryNode()
{
    clearInput();
}

void
InputGeometryNode::clearInput()
{
    delete myInput;
    myInput = NULL;
}

bool
InputGeometryNode::checkInput(MDataBlock &dataBlock)
{
    MPlug inputGeometryPlug(thisMObject(), InputGeometryNode::inputGeometry);

    Input::AssetInputType newAssetInputType = Input::AssetInputType_Invalid;
    {
        MObject geoData;

        MDataHandle geoDataHandle = dataBlock.inputValue(
                inputGeometryPlug);
        geoData = geoDataHandle.data();

        if(geoData.hasFn(MFn::kMeshData))
        {
            newAssetInputType = Input::AssetInputType_Mesh;
        }
        else if(geoData.hasFn(MFn::kNurbsCurveData))
        {
            newAssetInputType = Input::AssetInputType_Curve;
        }
        else if(geoData.hasFn(MFn::kVectorArrayData))
        {
            newAssetInputType = Input::AssetInputType_Particle;
        }
    }

    if(newAssetInputType == Input::AssetInputType_Invalid)
    {
        clearInput();
        return false;
    }

    // if the existing input doesn't match the new input type, delete it
    if(myInput && myInput->assetInputType() != newAssetInputType)
    {
        clearInput();
    }

    // create Input if necessary
    if(!myInput)
    {
        myInput = Input::createAssetInput(newAssetInputType);
    }

    if(!myInput)
    {
        clearInput();
        return false;
    }

    return true;
}
