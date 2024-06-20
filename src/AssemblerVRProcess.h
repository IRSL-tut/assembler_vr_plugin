#ifndef __ASSEMBLER_VR_PROCESS_H__
#define __ASSEMBLER_VR_PROCESS_H__

//// is it right way??
#include "../../robot_assembler_plugin/src/AssemblerManager.h"
#include "../../openvr_plugin/src/OpenVRPlugin.h"

#include <cnoid/SceneItem>

namespace cnoid {

class AssemblerVRProcess
{
public:
    AssemblerVRProcess();
    ~AssemblerVRProcess() {};

    void setLeftCoords(const coordinates &cds);
    void setRightCoords(const coordinates &cds);

    void updateControllerState(const controllerState &left, const controllerState &right);

private:
    AssemblerManager *as_manager;
    OpenVRPlugin *vr_plugin;

    SceneItemPtr leftHand;
    SceneItemPtr rightHand;
    //// bodyitem
};

}
#endif // __ASSEMBLER_VR_PROCESS_H__
