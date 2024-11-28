#ifndef __ASSEMBLER_VR_PROCESS_H__
#define __ASSEMBLER_VR_PROCESS_H__

//// is it right way??
#include "../../robot_assembler_plugin/src/AssemblerManager.h"
#include "../../openvr_plugin/src/OpenVRPlugin.h"

#include <cnoid/SceneItem>
#include <cnoid/SceneDrawables>

#include "exportdecl.h"

namespace cnoid {

class CNOID_EXPORT AssemblerVRProcess
{
public:
    AssemblerVRProcess();
    ~AssemblerVRProcess() {};

    void setLeftCoords(const coordinates &cds);
    void setRightCoords(const coordinates &cds);
    void grabrobot(ra::RASceneRobot* rb,const coordinates &hand);
    void updateControllerState(const controllerState &left, const controllerState &right);

    std::ostream *os_;

//private:
    AssemblerManager *as_manager;
    OpenVRPlugin *vr_plugin;

    SceneItemPtr leftHand;    SceneItemPtr rightHand;

    SgSwitchableGroupPtr left_switch_bm;
    SgSwitchableGroupPtr left_switch;
    SgScaleTransformPtr left_scale;

    SgSwitchableGroupPtr right_switch_bm;
    SgSwitchableGroupPtr right_switch;
    SgScaleTransformPtr right_scale;
    
    coordinates PreviousControllerCoords;
    robot_assembler::RASceneBase *obj = nullptr;
    robot_assembler::RASceneParts *pt_ = nullptr;
    robot_assembler::RASceneConnectingPoint *cp_ = nullptr;
    robot_assembler::RASceneConnectingPoint *cp_test = nullptr;
    robot_assembler::RASceneRobot *rb_ = nullptr;

    bool btn_flg = 0;
    //// bodyitem

    robot_assembler::RASceneBase *pick_object(const coordinates &cam_coords);
};

}
#endif // __ASSEMBLER_VR_PROCESS_H__
