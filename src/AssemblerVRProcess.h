#ifndef __ASSEMBLER_VR_PROCESS_H__
#define __ASSEMBLER_VR_PROCESS_H__

//// is it right way??
#include "../../robot_assembler_plugin/src/AssemblerManager.h"
#include "../../openvr_plugin/src/OpenVRPlugin.h"
#include "VR_UIbuttonGrid.h"
#include <cnoid/SceneItem>
#include <cnoid/SceneDrawables>
#include <cnoid/BodyItem>
#include <cnoid/Body>

#include <cnoid/GLSLSceneRenderer>
#include "exportdecl.h"

namespace cnoid {

class CNOID_EXPORT AssemblerVRProcess
{
public:
    AssemblerVRProcess(std::ostream *strm);
    ~AssemblerVRProcess() {};

    coordinates convertOpenGLToChoreonoid(const coordinates& openglCoords);
    void setLeftCoords(const coordinates &cds);
    void setRightCoords(const coordinates &cds);
    void grabrobot(ra::RASceneRobotPtr rb,const coordinates &hand ,const coordinates &prev_hand);
    void autoAttachNearbyParts(double threshold);
    void updateControllerState(const controllerState &left, const controllerState &right);
    void moveBodyItem(BodyItem* bodyItemPtr, const Vector3& translation);
    void move_body_byJoy(BodyItem *bodyItemPtr,double l_joy_x,double l_joy_y,double r_joy_x,double r_joy_y);
    BodyItemPtr findBodyItemFromNodePath(const SgNodePath& nodePath);
    BodyItemPtr AssemblerVRProcess::pick_object_body(const coordinates &cam_coords);
    SceneItemPtr pick_object_menu(const coordinates& cameraCoords, const VR_UIbuttonGrid* buttonGrid);
    SceneItemPtr search_SceneItem_for_button(GLSLSceneRenderer *glsr,const VR_UIbuttonGrid* buttonGrid);
    robot_assembler::RASceneBase *search_parts(GLSLSceneRenderer *glsr);
    GLSLSceneRenderer *pick_(const coordinates& cameraCoords);
    VR_UIbuttonGrid* AssemblerVRProcess::createButtonGrid(
        VR_UIbuttonGrid* ptr,
        std::string name,
        int rows,
        int cols,
        double spacing,
        const Vector3& buttonSize,
        const std::vector<std::string>& buttonNames,
        const std::vector<std::string>& texturePaths,
        const coordinates& offsetToCamera
    );
    void handleButtonPress(std::string button,coordinates vrcameracoords);
    void updateRightBeam(const coordinates& handCoords,GLSLSceneRenderer *glsr);
    void updateRedPointer(const Vector3& pickedPoint, bool hasIntersection);
    void updateRightBeamAndPointer(const coordinates& handCoords, GLSLSceneRenderer* glsr);
    void moveLastRobotInFrontOfHMD();
    std::ostream *os_;

//private:
    AssemblerManager *as_manager;
    OpenVRPlugin *vr_plugin;

    

    SceneItemPtr leftHand;
    SceneItemPtr rightHand;
    SceneItemPtr test_scene;
    SceneItemPtr red_ball = nullptr;
    std::string selected_parts_button_name = "";
    BodyItemPtr image_Dynamixel_XL;

    SgSwitchableGroupPtr left_switch_bm;
    SgSwitchableGroupPtr left_switch;
    SgScaleTransformPtr left_scale;

    SgSwitchableGroupPtr right_switch_bm;
    SgSwitchableGroupPtr right_switch;
    SgScaleTransformPtr right_scale;
    
    SgSwitchableGroupPtr image_Dynamixel_XL_switch;
    SgScaleTransformPtr image_Dynamixel_XL_scale;

    coordinates test_redball;
    coordinates PreviousControllerCoords_right;
    coordinates PreviousControllerCoords_left;
    robot_assembler::RASceneBase *obj = nullptr;
    robot_assembler::RASceneParts *pt_ = nullptr;
    robot_assembler::RASceneConnectingPoint *cp_ = nullptr;
    robot_assembler::RASceneConnectingPoint *cp_test = nullptr;
    robot_assembler::RASceneRobot *rb_ = nullptr;
    robot_assembler::RASceneRobot *rb_near_right = nullptr;
    robot_assembler::RASceneRobot *rb_near_left = nullptr;
    
    cnoid::VR_UIbuttonGrid* menu = nullptr;
    std::vector<VR_UIbuttonGrid*> menu_parts;
    VR_UIbuttonGrid* selected_menu = nullptr;
    VR_UIbuttonGrid* selected_menuparts = nullptr;
    SceneItemPtr picked_partsbutton = nullptr;
    bool VR_ASSEMBLER_MODE = 0;
    std::vector<std::string> menu_tab;
    bool btn_flg = 0;
    //// bodyitem

    robot_assembler::RASceneBase *pick_object(const coordinates &cam_coords);
};

}
#endif // __ASSEMBLER_VR_PROCESS_H__
