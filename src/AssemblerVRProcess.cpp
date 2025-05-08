#include "AssemblerVRProcess.h"
#include "VR_UIbuttonGrid.h"


#include <unordered_set> // 標準ライブラリのヘッダーを先にインクルード
#include <cnoid/RootItem>
#include <cnoid/MeshGenerator>
#include <cnoid/MessageView>
//// pick
#include <cnoid/SceneView>
#include <cnoid/SceneWidget>
#include <cnoid/SceneRenderer>
#include <cnoid/GLSLSceneRenderer>
#include <cnoid/SceneCameras>

#include <cnoid/BodyItem>
#include <cnoid/Body>
#include <cnoid/EigenUtil>

#define Assembler_mode 0
#define Parts_mode 1

using namespace cnoid;
using namespace cnoid::robot_assembler;


robot_assembler::RASceneBase *AssemblerVRProcess::pick_object(const coordinates &cam_coords)//画面真ん中のオブジェクトを拾ってくる
{
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT;
    cam_coords.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    //left_switch->setTurnedOn(false);
    right_switch->setTurnedOn(false);
    sw->builtinPerspectiveCamera()->setFieldOfView(0.02); //
    sw->makeCurrent();
    sr->pick(sw->width()/2, sw->height()/2);
    sw->doneCurrent();
    //left_switch->setTurnedOn(true);
    right_switch->setTurnedOn(true);

    sw->builtinCameraTransform()->setPosition(orgT);

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!!glsr) {
        //glsr->pickedNodePoint();
        const SgNodePath &np = glsr->pickedNodePath();
        const Vector3    &pt = glsr->pickedPoint();
        if (np.size() == 0) {
            return nullptr;
        }
        RASceneParts *pt_ = nullptr;
        RASceneConnectingPoint *cp_ = nullptr;
        for(auto n = np.begin(); n != np.end(); n++) {
            SgNode *ptr = *n;
            if(!pt_) pt_ = dynamic_cast<RASceneParts *>(ptr);
            if(!cp_) cp_ = dynamic_cast<RASceneConnectingPoint *>(ptr);
            if(!!pt_ && !!cp_) break;
        }
        if(!!cp_) {
            //connecting-point picked
            return cp_;
        } else if (!!pt_) {
            //parts picked
            return pt_;
        }
    }
    return nullptr;
}

AssemblerVRProcess::AssemblerVRProcess(std::ostream *strm)//VRでの初期処理一覧
{
    os_ = strm;
    as_manager = AssemblerManager::instance();//AssembleManegerのインスタンス
    vr_plugin  = OpenVRPlugin::instance();//OpenVRPluginのインスタンス
    {
        Item *p = RootItem::instance()->findItem("leftHand");//左手(名前がleftHand)のItemを見つけてくる
        if (!!p) {//Item_pが存在するとき
            leftHand = static_cast<SceneItem *>(p);//左手のオブジェクトに変換（キャスト）
            SgPosTransform *tp = leftHand->topNode();//lefthandのトップノードのポインタ
            left_scale = new SgScaleTransform();
            left_scale->setScale(0.2);//矢印の大きさを変える
            tp->moveChildrenTo(left_scale);
            left_switch = new SgSwitchableGroup();
            left_switch->addChild(left_scale);
            tp->addChild(left_switch);
        }
    }
    {
        Item *p = RootItem::instance()->findItem("rightHand");//右手(名前がrightHand)のItemを見つけてくる
        if (!!p) {
            //// register Item
            rightHand = static_cast<SceneItem *>(p);//右手のオブジェクトに変換（キャスト）
            //// making beam
            #define AXIS_LENGTH 30.0//軸の長さ（ビーム）
            right_switch_bm = new SgSwitchableGroup();
            SgPosTransform *trs = new SgPosTransform();
            SgShape *sph = new SgShape();
            { //// material
                SgMaterial *sgm = sph->getOrCreateMaterial();
                Vector3f diff(0., 1., 1.); //// color of axis(ビームの色)
                sgm->setDiffuseColor(diff);
                sgm->setAmbientIntensity(0.7);//周囲光強度
                sgm->setTransparency(0.6);//透明性
            }
            { //// mesh
                Vector3 size(0.005, 0.005, AXIS_LENGTH);//ビームサイズ（細長い直方体）
                MeshGenerator mg;
                sph->setMesh(mg.generateBox(size));
            }
            sph->setName("right beam");//オブジェクトの名前
            trs->setTranslation(Vector3(0, 0, AXIS_LENGTH*-0.5));
            trs->addChild(sph);
            right_switch_bm->addChild(trs);
            rightHand->topNode()->addChild(right_switch_bm);
            right_switch_bm->setTurnedOn(true);
            rightHand->topNode()->notifyUpdate(SgUpdate::Modified);
            ////
            //// insert scale between transform and shape
            SgPosTransform *tp = rightHand->topNode();
            right_scale = new SgScaleTransform();
            right_scale->setScale(0.2);
            tp->moveChildrenTo(right_scale);
            right_switch = new SgSwitchableGroup();
            right_switch->setName("right_switch");
            right_switch->addChild(right_scale);
            tp->addChild(right_switch);
        }
    }
    {
        // Item *p = RootItem::instance()->findItem("image_Dynamixel_XL");
        // if(!!p){
        //     image_Dynamixel_XL = static_cast<BodyItem *>(p);
        //     Link* rootLink = image_Dynamixel_XL->body()->rootLink();
        //     Isometry3 currentTransform = rootLink->position();
        //     Matrix3d rotation;
        //     rotation << 1, 0,  0,
        //                 0, 0, -1,
        //                 0, 1,  0;
        //     currentTransform.linear() = rotation;
        //     rootLink->setPosition(currentTransform);
        //     image_Dynamixel_XL->notifyKinematicStateChange(true);
        // }
    }
    ////メニューの作成/////////////////////////////////////////////////////////////
    // ボタンの設定
    int rows = 1; // ボタンの行数
    int cols = 7; // ボタンの列数
    double spacing = 0.2; // ボタン間のスペース(ボタンの大きさとそろえると敷き詰めれる)
    Vector3 buttonSize(0.2, 0.2, 0.01); // ボタンのサイズ (x, y, z)
    Vector3 imageSize(0.4, 0.4, 0.01);
    // ボタンの名前
    std::vector<std::string> buttonNames = {
        "Servo_Parts", "Frames", "Vehicle_Parts", "Sensor_Parts",
        "Special_Parts", "Main_Blocks", "Main_Plates"
    };
    std::vector<std::string> texturePaths = {
        "../texture/Servo_Parts.png", "../texture/Frames.png","../texture/Vehicle_Parts.png", "../texture/Sensor_Parts.png",
        "../texture/Special_Parts.png", "../texture/Main_Blocks.png","../texture/Main_Plates.png"
    };
    coordinates menu_offset;
    menu_offset.pos = Vector3(1.3, -0.5, -0.52);
    //menu_offset.pos = Vector3(-0.6, -0.5, 0.0);
    Matrix3 rot;
    rot <<  0, 0,  -1,
            1, 0,  0,
            0, -1, 0;

    menu_offset.rot = rot;
    menu = createButtonGrid(menu,"menu_bar",rows,cols,spacing,buttonSize,buttonNames,texturePaths,menu_offset);
    // menu->createDescriptionImage("../texture/Servo_Parts.png", imageSize, -0.5);
    // menu->updateDescriptionTexture("../texture/Special_Parts.png");
    menu->setVisible(true);//メニューバーを非表示

    menu->setVisible_image(false);//メニューバーを非表示
    //アイテムバーの作成///////////////////////////////////////////////////////////////
    menu_tab = {
        "Servo_Parts", "Frames", "Vehicle_Parts", "Sensor_Parts",
        "Special_Parts", "Main_Blocks", "Main_Plates"
    };
    std::vector<std::vector<std::string>> item_list= {
        {"XL-430", "Hinge_Short_Plug", "Hinge_Short_Socket", "Hinge_Long_Plug", "Hinge_Long_Socket",  "omni-shaft-x", "Moveable_Finger",""},
        {"Frame_Short_Plug", "Frame_Short_Socket", "Frame_Short", "Frame_Long_Plug", "Frame_Long_Socket", "Frame_Bottom_Plug", "Frame_Bottom_Socket", "Fixed_Finger"},
        {"vehicle-base-pp-r1", "vehicle-base-ps", "omni-shaft-x", "omni-wheel"},
        {"sensor-mounter", "grove-sensor-mounter", "ultrasonic-sensor", "tof-sensor", "grove-color-sensor", "camera-module"},
        {"angle_2_2", "plate_2_2_pp", "diagonal_bracket",""},
        {"block_2_2", "block_2_3", "block_2_4", "block_2_6", "block_2_8", "block_2_10",
            "block_1_1", "block_1_2", "block_1_3", "block_1_4", "block_1_6", "block_1_8", "block_1_10",""},
        {"plate_2_2" , "plate_2_3", "plate_2_4", "plate_2_6", "plate_2_8", "plate_2_10",
            "plate_1_1", "plate_1_2", "plate_1_3", "plate_1_4", "plate_1_6", "plate_1_8",
            "plate_4_4", "plate_4_8", "plate_6_12",""}
    };
    std::vector<std::vector<std::string>> texture_item_list= {
        {"../texture/Servo_Parts/XL-430.png", "../texture/Servo_Parts/Hinge_Short_Plug.png", "../texture/Servo_Parts/Hinge_Short_Socket.png", "../texture/Servo_Parts/Hinge_Long_Plug.png", "../texture/Servo_Parts/Hinge_Long_Socket.png",  "../texture/Servo_Parts/omni-shaft-x.png", "../texture/Servo_Parts/Moveable_Finger.png",""},
        {"../texture/Frames/Frame_Short_Plug.png", "../texture/Frames/Frame_Short_Socket.png", "../texture/Frames/Frame_Short.png", "../texture/Frames/Frame_Long_Plug.png", "../texture/Frames/Frame_Long_Socket.png", "../texture/Frames/Frame_Bottom_Plug.png", "../texture/Frames/Frame_Bottom_Socket.png", "../texture/Frames/Fixed_Finger.png"},
        {"../texture/Vehicle_Parts/vehicle-base-pp-r1.png", "../texture/Vehicle_Parts/vehicle-base-ps.png", "../texture/Vehicle_Parts/omni-shaft-x.png", "../texture/Vehicle_Parts/omni-wheel.png"},
        {"../texture/Sensor_Parts/sensor-mounter.png", "../texture/Sensor_Parts/grove-sensor-mounter.png", "../texture/Sensor_Parts/ultrasonic-sensor.png", "../texture/Sensor_Parts/tof-sensor.png", "../texture/Sensor_Parts/grove-color-sensor.png", "../texture/Sensor_Parts/camera-module.png"},
        {"../texture/Special_Parts/angle_2_2.png", "../texture/Special_Parts/plate_2_2_pp.png", "../texture/Special_Parts/diagonal_bracket.png",""},
        {"../texture/Main_Blocks/block_2_2.png", "../texture/Main_Blocks/block_2_3.png", "../texture/Main_Blocks/block_2_4.png", "../texture/Main_Blocks/block_2_6.png", "../texture/Main_Blocks/block_2_8.png", "../texture/Main_Blocks/block_2_10.png",
            "../texture/Main_Blocks/block_1_1.png", "../texture/Main_Blocks/block_1_2.png", "../texture/Main_Blocks/block_1_3.png", "../texture/Main_Blocks/block_1_4.png", "../texture/Main_Blocks/block_1_6.png", "../texture/Main_Blocks/block_1_8.png", "../texture/Main_Blocks/block_1_10.png",""},
        {"../texture/Main_Plates/plate_2_2.png" , "../texture/Main_Plates/plate_2_3.png", "../texture/Main_Plates/plate_2_4.png", "../texture/Main_Plates/plate_2_6.png", "../texture/Main_Plates/plate_2_8.png", "../texture/Main_Plates/plate_2_10.png",
            "../texture/Main_Plates/plate_1_1.png", "../texture/Main_Plates/plate_1_2.png", "../texture/Main_Plates/plate_1_3.png", "../texture/Main_Plates/plate_1_4.png", "../texture/Main_Plates/plate_1_6.png", "../texture/Main_Plates/plate_1_8.png",
            "../texture/Main_Plates/plate_4_4.png", "../texture/Main_Plates/plate_4_8.png", "../texture/Main_Plates/plate_6_12.png",""}
    };
    std::vector<std::pair<Vector3, Matrix3>> offset_list = {
        {Vector3(1.3, -0.2, -0.3), rot},//pos、単位行列で初期化
        {Vector3(1.3, -0.2, -0.3), rot},
        {Vector3(1.3, -0.2, -0.3), rot},
        {Vector3(1.3, -0.2, -0.3), rot},
        {Vector3(1.3, -0.2, -0.3), rot},
        {Vector3(1.3, -0.2, -0.3), rot},
        {Vector3(1.3, -0.2, -0.3), rot}
    };
    std::vector<coordinates> offset_list_parts;
    for(int i=0;i<offset_list.size();i++){
        coordinates set;
        set.pos = offset_list[i].first;
        set.rot = offset_list[i].second;
        offset_list_parts.push_back(set);
    }
    rows = 2;//2行
    spacing = 0.2; // ボタン間のスペース(ボタンの大きさとそろえると敷き詰めれる)
    for(int k = 0; k<menu_tab.size(); k++){
        menu_parts.push_back(nullptr);
        cols = item_list[k].size()/2;
        menu_parts[k] = createButtonGrid(menu_parts[k],menu_tab[k],rows,cols,spacing,buttonSize,item_list[k],texture_item_list[k],offset_list_parts[k]);
        menu_parts[k]->createDescriptionImage("../discription_image/Servo_Parts/XL-430.png", imageSize, -0.5);
        menu_parts[k]->setVisible(false);//メニューバーを非表示
    }
    
    ////////////////////////////////////////////////////////////////////////////
    if(!!vr_plugin) {//コントローラの情報取得
        vr_plugin->sigUpdateControllerState().connect(std::bind(&AssemblerVRProcess::updateControllerState, this,
                                                                std::placeholders::_1, std::placeholders::_2));
    }

    if(!!vr_plugin) {
        vr_plugin->setProjectionMatrix(2);
        vr_plugin->setEyeDifferenceScale(1/2);
    }
}


void AssemblerVRProcess::updateControllerState(const controllerState &right, const controllerState &left)//コントローラの状態更新(入れ替わってたら逆にする)
{  
    setLeftCoords(left.coords);
    setRightCoords(right.coords);
    test_redball=left.coords;
    coordinates right_coords =  convertOpenGLToChoreonoid(right.coords);
    coordinates left_coords =  convertOpenGLToChoreonoid(left.coords);

    coordinates camera = vr_plugin->CameraOrigin();

    //カメラのcoords取得
    coordinates vrcameracoords = convertOpenGLToChoreonoid(vr_plugin->CameraOrigin());
    if(red_ball){red_ball->setChecked(false);}
    GLSLSceneRenderer *glsr = pick_(right.coords);//画面の真ん中を引っ張ってくる。
    if(red_ball){red_ball->setChecked(true);}
    Vector3 pickedPoint = glsr->pickedPoint();
    //ボタンの事前処理
    static bool previous_button_state_r[5] = {false,false,false,false,false};
    static bool previous_button_state_l[5] = {false,false,false,false,false};

    
    
    //updateRightBeam(right.coords,glsr);
    updateRightBeamAndPointer(right.coords,glsr);
    if (!!as_manager) {
        // Assembler_mode 0
        // Parts_mode 1
        if(VR_ASSEMBLER_MODE == Assembler_mode){
            obj = search_parts(glsr);
            if(!!obj){
                pt_  = dynamic_cast<RASceneParts *>(obj);
                if(!!pt_){
                    rb_ = pt_->scene_robot();//ptのRASceneRobotを取得
                }
                cp_ = dynamic_cast<RASceneConnectingPoint *>(obj);
                if(!!cp_){
                    vr_plugin->causeVive(500);//500ms
                }
            }

            if(right.buttons[0] && !previous_button_state_r[0]){//セレクトA(select_cp)
                if(!!cp_){
                    as_manager->selectRobot(cp_->scene_robot());
                    as_manager->pointClicked(cp_);
                }
            }

            if(right.buttons[1] && !previous_button_state_r[1]){//セレクトB(align)
                as_manager->attachRobots(true,true,1);
            }
            
            if(left.buttons[0] && !previous_button_state_l[0]){//セレクトX(attach)
                as_manager->attachRobots(true,false,1);
            }
            
            if(left.buttons[1] && !previous_button_state_l[1]){//セレクトY
                if(left.buttons[4]){
                    as_manager->deleteRobot(rb_);
                }
                else{
                    if(!!pt_){
                        as_manager->detachSceneRobot(pt_);
                    }
                }
            }

            if (right.buttons[3]) {//移動・回転モード裏大
                if(!previous_button_state_r[3]){
                    rb_near = as_manager->searchNearest(right.coords.pos, 0.1);
                }
                if(!!rb_near){
                    left_switch->setTurnedOn(false);
                    right_switch->setTurnedOn(false);
                    grabrobot(rb_near,right.coords);
                }
            }
            else{
                left_switch->setTurnedOn(true);
                right_switch->setTurnedOn(true);
            }
        }
        if(VR_ASSEMBLER_MODE == Parts_mode){

            //カメラに追従する
            //vr_plugin->CameraOrigin()
            menu->setPosition_camera(vrcameracoords);
            
            
            if(!!selected_menu){
                selected_menu->setPosition_camera(vrcameracoords);
            }
            SceneItemPtr picked_menubutton = search_SceneItem_for_button(glsr,menu);
            if(!!picked_menubutton){//何かしらのパーツメニューが表示されているとき
                menu->setSelectedButton(picked_menubutton);
                handleButtonPress(picked_menubutton->name(),vrcameracoords);//パーツメニューを検索し、表示する。
            }
            if(!!selected_menu){
                picked_partsbutton = search_SceneItem_for_button(glsr,selected_menu);
                if(!!picked_partsbutton){
                    selected_menu->setSelectedButton(picked_partsbutton);
                    if(selected_parts_button_name != selected_menu->getselected_button()){
                        selected_menu->updateDescriptionTexture();
                    }
                selected_parts_button_name = selected_menu->getselected_button();
                }
            }
            if (right.buttons[3] && !previous_button_state_r[3]) {//rightの裏大トリガー
                if(!!picked_partsbutton){
                    as_manager->partsButtonClicked(picked_partsbutton->name());
                    vr_plugin->causeVive(500);
                }
            }
        }
        // //終了処理
        if(left.buttons[3] && !previous_button_state_l[3]){
            *os_ << "assembler:mode changed"<<std::endl;
            if(VR_ASSEMBLER_MODE == Assembler_mode){
                VR_ASSEMBLER_MODE = Parts_mode;
                menu->setVisible(true);//メニューバーを表示
                // RootItemからすべての子アイテムを取得
                for (Item* item = RootItem::instance()->childItem(); item; item = item->nextItem()) {
                    const std::string itemName = item->name();
                    const std::string className = typeid(*item).name();
                    if(item->name()=="Table"||typeid(*item).name()=="cnoid::AssemblerItem"){item->setChecked(false);}
                    *os_ << "cls" <<className<<std::endl;
                }
            }
            else{
                VR_ASSEMBLER_MODE = Assembler_mode;
                menu->setVisible(false);//メニューバーを非表示
                if(!!selected_menu){
                    selected_menu->setVisible(false);//メニューバーを非表示
                    selected_menu->setVisible_image(false);
                }
                for (Item* item = RootItem::instance()->childItem(); item; item = item->nextItem()) {
                    const std::string itemName = item->name();
                    const std::string className = typeid(*item).name();
                    if(item->name()=="Table"||typeid(*item).name()=="cnoid::AssemblerItem"){item->setChecked(true);}
                    *os_ << "cls" <<className<<std::endl;
                }
            }
        }

        PreviousControllerCoords = right.coords;//コントローラのcoords保持
        //ボタンの終了処理
        for(int i=0;i<right.buttons.size();i++){
            previous_button_state_r[i] = right.buttons[i];
            previous_button_state_l[i] = left.buttons[i];
        }

    }

    
    
    //move_body_byJoy(image_Dynamixel_XL,left.axes[0],left.axes[1],right.axes[0],right.axes[1]);
    return;
}

void cnoid::AssemblerVRProcess::moveBodyItem(BodyItem *bodyItemPtr,const Vector3 &translation)
{
    Link* rootLink = bodyItemPtr->body()->rootLink();
    Position currentPosition = rootLink->position();
    currentPosition.translation() = translation;
    rootLink->setPosition(currentPosition);
    bodyItemPtr->notifyKinematicStateChange(true);
}
// void cnoid::AssemblerVRProcess::follow_body_toCamera(BodyItem *bodyItemPtr){//作りかけ
//     coordinates camera_coords = vr_plugin->CameraOrigin();
//     Link* rootLink = bodyItemPtr->body()->rootLink();
//     Isometry3 currentTransform = rootLink->position();
    
// }
void cnoid::AssemblerVRProcess::move_body_byJoy(BodyItem *bodyItemPtr,double l_joy_x,double l_joy_y,double r_joy_x,double r_joy_y){
    Link* rootLink = bodyItemPtr->body()->rootLink();
    Isometry3 currentTransform = rootLink->position();
    double translationSpeed = 0.05;
    double rotationSpeed = 0.5;
    // 移動計算
    Vector3 translation = Vector3::Zero();
    translation[0] = l_joy_x * translationSpeed; // 左スティックX軸でX方向移動
    translation[1] = l_joy_y * translationSpeed; // 左スティックY軸でY方向移動
    translation[2] = r_joy_y * translationSpeed; // 右スティックY軸でZ方向移動
    currentTransform.translation() += translation;
    /////回転処理////
    // double rotationAngle = (-1)*r_joy_x * rotationSpeed; // 右スティックX軸でZ軸回転
    // AngleAxisd rotationZ(rotationAngle, Vector3::UnitY()); // Y軸回転行列
    // Matrix3 newRotation = currentTransform.rotation() * rotationZ.toRotationMatrix();
    // 更新した位置と回転をリンクに適用
    //currentTransform.linear() = newRotation;
    rootLink->setPosition(currentTransform);
    bodyItemPtr->notifyKinematicStateChange(true);
}

BodyItemPtr cnoid::AssemblerVRProcess::findBodyItemFromNodePath(const SgNodePath &nodePath)
{
    for (Item* item = RootItem::instance()->childItem(); item; item = item->nextItem()) {
        if (auto bodyItem = dynamic_cast<BodyItem*>(item)) {
            SgNode* bodyScene = bodyItem->getScene();
            if (!bodyScene) {
                continue; // Sceneが取得できない場合は次のItemへ
            }

            // nodePath内のノードがbodySceneの子ノードに含まれているか確認
            for (SgNode* node : nodePath) {
                SgObject* current = node;
                while (current) {
                    if (current == bodyScene) {
                        return bodyItem; // BodyItemを返す
                    }

                    // 親ノードを取得
                    auto parentIter = current->parentBegin();
                    current = (parentIter != current->parentEnd()) ? *parentIter : nullptr;
                }
            }
        }
    }
    return nullptr; // 該当するBodyItemが見つからない場合
}


void AssemblerVRProcess::grabrobot(ra::RASceneRobotPtr rb,const coordinates &hand){
    coordinates diff,set;
    coordinates cds(rb->T());//objのcoordsを取得
    set = hand;
    diff.pos = hand.pos - PreviousControllerCoords.pos;//コントローラの位置変化量
    set.pos = cds.pos + diff.pos;//差分位置の加算
    Matrix3 rotationMatrix = hand.rot * PreviousControllerCoords.rot.transpose();
    set.rot =rotationMatrix * cds.rot;
    as_manager->selectRobot(rb);
    //as_manager->move_robot(rb,set);
    as_manager->moveRobot(rb,set);
}



void AssemblerVRProcess::setLeftCoords(const coordinates &cds)//左手の座標セット
{   
    if (!!leftHand) {
        Isometry3 T;
        cds.toPosition(T);
        leftHand->topNode()->setPosition(T);
    }
}

void AssemblerVRProcess::setRightCoords(const coordinates &cds)//右手の座標セット
{
    if (!!rightHand) {
        Isometry3 T;
        cds.toPosition(T);
        rightHand->topNode()->setPosition(T);
    }
}

BodyItemPtr AssemblerVRProcess::pick_object_body(const coordinates &cam_coords)//画面真ん中のオブジェクトを拾ってくる
{
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT;
    cam_coords.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    //left_switch->setTurnedOn(false);
    right_switch->setTurnedOn(false);
    sw->builtinPerspectiveCamera()->setFieldOfView(0.02); //
    sw->makeCurrent();
    sr->pick(sw->width()/2, sw->height()/2);
    sw->doneCurrent();
    //left_switch->setTurnedOn(true);
    right_switch->setTurnedOn(true);

    sw->builtinCameraTransform()->setPosition(orgT);

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!!glsr) {
        //glsr->pickedNodePoint();
        const SgNodePath &np = glsr->pickedNodePath();
        const Vector3    &pt = glsr->pickedPoint();
        if (np.size() == 0) {
            return nullptr;
        }
        return findBodyItemFromNodePath(np);
    }
    return nullptr;
}

SceneItemPtr AssemblerVRProcess::pick_object_menu(const coordinates& cameraCoords, const VR_UIbuttonGrid* buttonGrid) {
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT;
    cameraCoords.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    //left_switch->setTurnedOn(false);
    right_switch->setTurnedOn(false);
    sw->builtinPerspectiveCamera()->setFieldOfView(0.02); //
    sw->makeCurrent();
    sr->pick(sw->width()/2, sw->height()/2);
    
    sw->doneCurrent();
    //left_switch->setTurnedOn(true);
    right_switch->setTurnedOn(true);

    sw->builtinCameraTransform()->setPosition(orgT);

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!glsr) {
        return nullptr;
    }

    const SgNodePath& pickedNodePath = glsr->pickedNodePath();
    if (pickedNodePath.empty()) {
        return nullptr;
    }

    // ノードパスをハッシュセットに変換
    std::unordered_set<const SgNode*> pickedNodes(pickedNodePath.begin(), pickedNodePath.end());

    // ボタンリストから該当するノードを検索
    const auto& buttons = buttonGrid->getButtons();
    for (const auto& [item, transform] : buttons) {
        if (transform) {
            auto it = pickedNodes.find(transform);
            if (it != pickedNodes.end()) {
                return item;
            } 
        }
    }
    return nullptr;
}

coordinates AssemblerVRProcess::convertOpenGLToChoreonoid(const coordinates& openglCoords)//右手の座標セット
{   
    coordinates coords;
     // 座標変換行列 T
    Eigen::Matrix3d T;
    T <<  0,  0,  -1,
          -1,  0,  0,
          0,  1, 0;

    // 位置ベクトルの変換
    Vector3 choreonoidPos = T * openglCoords.pos;
    coords.pos = choreonoidPos;
    // 回転行列の変換
    Matrix3 choreonoidRot = T * openglCoords.rot * T.transpose();
    coords.rot = choreonoidRot;
    // 変換結果を返す
    // Choreonoidの座標を生成
    return coords;
}

GLSLSceneRenderer *AssemblerVRProcess::pick_(const coordinates& cameraCoords){
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT;
    cameraCoords.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    //left_switch->setTurnedOn(false);
    right_switch->setTurnedOn(false);
    sw->builtinPerspectiveCamera()->setFieldOfView(0.02); //
    sw->makeCurrent();
    sr->pick(sw->width()/2, sw->height()/2);
    
    sw->doneCurrent();
    //left_switch->setTurnedOn(true);
    right_switch->setTurnedOn(true);

    sw->builtinCameraTransform()->setPosition(orgT);

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!glsr) {
        return nullptr;
    }

    return glsr;
}

SceneItemPtr AssemblerVRProcess::search_SceneItem_for_button(GLSLSceneRenderer *glsr,const VR_UIbuttonGrid* buttonGrid){
    const SgNodePath& pickedNodePath = glsr->pickedNodePath();
    if (pickedNodePath.empty()) {
        return nullptr;
    }

    // ノードパスをハッシュセットに変換
    std::unordered_set<const SgNode*> pickedNodes(pickedNodePath.begin(), pickedNodePath.end());

    // ボタンリストから該当するノードを検索
    const auto& buttons = buttonGrid->getButtons();
    for (const auto& [item, transform] : buttons) {
        if (transform) {
            auto it = pickedNodes.find(transform);
            if (it != pickedNodes.end()) {
                return item;
            } 
        }
    }
    return nullptr;

}

robot_assembler::RASceneBase *cnoid::AssemblerVRProcess::search_parts(GLSLSceneRenderer *glsr)
{
    if (!!glsr) {
        //glsr->pickedNodePoint();
        const SgNodePath &np = glsr->pickedNodePath();
        const Vector3    &pt = glsr->pickedPoint();
        if (np.size() == 0) {
            return nullptr;
        }
        RASceneParts *pt_ = nullptr;
        RASceneConnectingPoint *cp_ = nullptr;
        for(auto n = np.begin(); n != np.end(); n++) {
            SgNode *ptr = *n;
            if(!pt_) pt_ = dynamic_cast<RASceneParts *>(ptr);
            if(!cp_) cp_ = dynamic_cast<RASceneConnectingPoint *>(ptr);
            if(!!pt_ && !!cp_) break;
        }
        if(!!cp_) {
            //connecting-point picked
            return cp_;
        } else if (!!pt_) {
            //parts picked
            return pt_;
        }
    }
    return nullptr;
}


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
) {
    // ボタンのグリッドを作成
    auto* buttonGrid = new VR_UIbuttonGrid(name,rows, cols, spacing, buttonSize);

    // ボタンを生成
    buttonGrid->createButtons(buttonNames, texturePaths);

    // カメラとのオフセットを設定
    buttonGrid->setOffsetToCamera(offsetToCamera);

    return buttonGrid;
}

void AssemblerVRProcess::handleButtonPress(std::string pressed_button_name,coordinates vrcameracoords) {

    // 名前に基づいて対応するインデックスを検索
    auto it = std::find(menu_tab.begin(), menu_tab.end(), pressed_button_name);
    if (it == menu_tab.end()) {//見つからなかったら
        return;
    }

    int index = std::distance(menu_tab.begin(), it);

    // 対応するボタン群を取得
    VR_UIbuttonGrid* corresponding_menu = menu_parts[index];
    if (!corresponding_menu) {//見つからなかったら
        return;
    }

    selected_menu = corresponding_menu;
    // ボタン群を表示
    corresponding_menu->setVisible(true);
    corresponding_menu->setVisible_image(true);
    // 他のボタン群を非表示にする
    for (size_t i = 0; i < menu_parts.size(); ++i) {
        if (i != index && menu_parts[i]) {
            menu_parts[i]->setVisible(false);
            menu_parts[i]->setVisible_image(false);
        }
    }
}

void AssemblerVRProcess::updateRightBeam(const coordinates& handCoords,GLSLSceneRenderer *glsr) {
    if (!right_switch_bm) {
        return; // ビームが初期化されていない場合は処理を中断
    }

    // ビームの形状を取得
    SgPosTransform* trs = dynamic_cast<SgPosTransform*>(right_switch_bm->child(0));
    if (!trs || trs->numChildren() == 0) {
        return; // ビームの形状が見つからない場合は処理を中断
    }

    SgShape* beamShape = dynamic_cast<SgShape*>(trs->child(0));
    if (!beamShape) {
        return; // ビームの形状が正しく設定されていない場合は処理を中断
    }

    // ビームのデフォルト長さと交差点情報を設定
    double beamLength = AXIS_LENGTH;
    Vector3 beamEnd = handCoords.pos + handCoords.rot * Vector3(0, 0, -beamLength);
    Vector3 pickedPoint; // 定義を追加

    // GLSLSceneRendererで交差判定
    bool hasIntersection = false; // 交差判定フラグ
    if (glsr) {
        const SgNodePath& nodePath = glsr->pickedNodePath();
        if (!nodePath.empty()) {
            // オブジェクトにヒットした場合、交点の位置を取得
            pickedPoint = glsr->pickedPoint();
            beamLength = (pickedPoint - handCoords.pos).norm();
            beamEnd = pickedPoint; // ビームの終点を交点に設定
            hasIntersection = true;
        }
    }

    // ビームの形状を更新
    trs->setTranslation(Vector3(0, 0, -beamLength / 2.0));

    // 新しいメッシュを生成して更新
    MeshGenerator mg;
    Vector3 size(0.005, 0.005, beamLength);
    SgMeshPtr newMesh = mg.generateBox(size);
    beamShape->setMesh(newMesh);


    *os_ <<"pickedpoint_after"<<pickedPoint<<std::endl;
    //updateRedPointer(pickedPoint, hasIntersection);

    rightHand->topNode()->notifyUpdate(SgUpdate::Modified);
}

void AssemblerVRProcess::updateRedPointer(const Vector3& pickedPoint, bool hasIntersection) {
    static bool highlightVisible = false;
    static SgPosTransform* highlightTransform = nullptr;

    // RootItem のインスタンスを取得
    auto rootItem = RootItem::instance();
    if (!rootItem) {
        return; // RootItemが見つからない場合は処理を中断
    }

    // 初回呼び出し時に赤いポインタを作成
    if (!highlightTransform) {
        highlightTransform = new SgPosTransform();
        SgShape* sphere = new SgShape();
        sphere->setName("HighlightPoint");

        // 赤いマテリアルを設定
        SgMaterial* material = sphere->getOrCreateMaterial();
        material->setDiffuseColor(Vector3f(0.0f, 1.0f, 1.0f)); // 黄緑色
        material->setAmbientIntensity(0.7f);
        material->setTransparency(0.0f); // 完全に不透明

        // 球体のメッシュを生成
        MeshGenerator mg;
        sphere->setMesh(mg.generateSphere(0.005)); // 半径 0.5 の球体
        highlightTransform->addChild(sphere);

        // SceneItem を生成し、RootItem に直接追加
        auto sceneItem = new SceneItem();
        sceneItem->setName("HighlightPointScene");
        sceneItem->topNode()->addChild(highlightTransform);

        // RootItem に SceneItem を追加
        rootItem->addChildItem(sceneItem);
    }

    if (hasIntersection) {
        // 赤いポインタを交差点に移動
        highlightTransform->setTranslation(pickedPoint);
        if (!highlightVisible) {
            highlightVisible = true;
        }
    } else if (highlightVisible) {
        // 赤いポインタを非表示（削除はしない）
        highlightVisible = false;
    }

    // 更新を通知
    highlightTransform->notifyUpdate(SgUpdate::Modified);
}

void AssemblerVRProcess::updateRightBeamAndPointer(const coordinates& handCoords, GLSLSceneRenderer* glsr) {
    static SgPosTransform* highlightTransform = nullptr;

    // 初回呼び出し時に赤いポインタを作成
    if (!highlightTransform) {
        highlightTransform = new SgPosTransform();
        SgShape* sphere = new SgShape();
        sphere->setName("HighlightPoint");

        // 赤いマテリアルを設定
        SgMaterial* material = sphere->getOrCreateMaterial();
        material->setDiffuseColor(Vector3f(1.0f, 0.0f, 0.0f)); // 赤色
        material->setAmbientIntensity(0.7f);
        material->setTransparency(0.0f); // 完全に不透明

        // 球体のメッシュを生成
        MeshGenerator mg;
        sphere->setMesh(mg.generateSphere(0.0025)); // 半径 0.5 の球体
        highlightTransform->addChild(sphere);

        // SceneItem を生成し、RootItem に直接追加
        red_ball = new SceneItem();
        red_ball->setName("HighlightPointScene");
        red_ball->topNode()->addChild(highlightTransform);

        RootItem::instance()->addChildItem(red_ball);
    }

    
    // if (red_ball) {
    //     red_ball->setChecked(false); // 非表示
    // }

    Vector3 pickedPoint;
    bool hasIntersection = false;

    // GLSLSceneRendererで交差判定
    if (glsr) {
        const SgNodePath& nodePath = glsr->pickedNodePath();
        if (!nodePath.empty()) {
            pickedPoint = glsr->pickedPoint();
            hasIntersection = true;
        }
    }

    // // 赤いポインタを再表示
    // if (red_ball) {
    //     red_ball->setChecked(true); // 再表示
    // }

    if (hasIntersection) {
        // 赤いポインタを交差点に移動
        highlightTransform->setTranslation(pickedPoint);
    }

    // 更新を通知
    highlightTransform->notifyUpdate(SgUpdate::Modified);
}