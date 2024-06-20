#include "AssemblerVRProcess.h"

#include <cnoid/RootItem>

using namespace cnoid;

AssemblerVRProcess::AssemblerVRProcess()
{
    as_manager = AssemblerManager::instance();
    vr_plugin  = OpenVRPlugin::instance();

    {
        Item *p = RootItem::instance()->findItem("leftHand");
        if (!!p) {
            leftHand = static_cast<SceneItem *>(p);
        }
    }
    {
        Item *p = RootItem::instance()->findItem("rightHand");
        if (!!p) {
            rightHand = static_cast<SceneItem *>(p);
        }
    }

    if(!!vr_plugin) {
        vr_plugin->sigUpdateControllerState().connect(std::bind(&AssemblerVRProcess::updateControllerState, this,
                                                                std::placeholders::_1, std::placeholders::_2));
    }
}

void AssemblerVRProcess::updateControllerState(const controllerState &left, const controllerState &right)
{
    setLeftCoords(left.coords);
    setRightCoords(right.coords);

    //// sample xxx
    if (!!as_manager) {
        robot_assembler::RASceneRobot *rb = as_manager->searchNearest(right.coords.pos, 0.1);
        if (!!rb) {
            as_manager->selectRobot(rb);
            //
            as_manager->moveRobot(rb, right.coords);
        }
    }

    return;
}

void AssemblerVRProcess::setLeftCoords(const coordinates &cds)
{
    if (!!leftHand) {
        Isometry3 T;
        cds.toPosition(T);
        leftHand->topNode()->setPosition(T);
    }
}

void AssemblerVRProcess::setRightCoords(const coordinates &cds)
{
    if (!!rightHand) {
        Isometry3 T;
        cds.toPosition(T);
        rightHand->topNode()->setPosition(T);
    }
}
