#include "AssemblerVRProcess.h"

#include <cnoid/RootItem>
#include <cnoid/MeshGenerator>

//// pick
#include <cnoid/SceneView>
#include <cnoid/SceneWidget>
#include <cnoid/SceneRenderer>
#include <cnoid/GLSLSceneRenderer>
#include <cnoid/SceneCameras>

using namespace cnoid;
using namespace cnoid::robot_assembler;

robot_assembler::RASceneBase *AssemblerVRProcess::pick_object(coordinates &cam_coords)
{
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT;
    cam_coords.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    left_switch->setTurnedOn(false);
    right_switch->setTurnedOn(false);
    sw->builtinPerspectiveCamera()->setFieldOfView(0.02); //
    sw->makeCurrent();
    sr->pick(sw->width()/2, sw->height()/2);
    sw->doneCurrent();
    left_switch->setTurnedOn(true);
    right_switch->setTurnedOn(true);

    sw->builtinCameraTransform()->setPosition(orgT);

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!!glsr) {
        //glsr->pickedNodePoint();
        const SgNodePath &np = glsr->pickedNodePath();
        const Vector3    &pt = glsr->pickedPoint();
        *os_ << "point : " << pt.x() << ", " << pt.y() << ", " << pt.z() << std::endl;
        *os_ << "NodePath : size : " << np.size() << std::endl;
        if (np.size() == 0) {
            return nullptr;
        }
        RASceneParts *pt_ = nullptr;
        RASceneConnectingPoint *cp_ = nullptr;
        for(auto n = np.begin(); n != np.end(); n++) {
            SgNode *ptr = *n;
            *os_ << "  name:" << (*n)->name();
            *os_ << ", cls:" << (*n)->className() << std::endl;
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

AssemblerVRProcess::AssemblerVRProcess()//VRでの処理一覧
{
    as_manager = AssemblerManager::instance();//AssembleManegerのインスタンス
    vr_plugin  = OpenVRPlugin::instance();//OpenVRPluginのインスタンス

    {
        Item *p = RootItem::instance()->findItem("leftHand");//左手(名前がleftHand)のItemを見つけてくる
        if (!!p) {//Item_pが存在するとき
            leftHand = static_cast<SceneItem *>(p);//左手のオブジェクトに変換（キャスト）
            //// making beam(left hund)
            ////
            ////
            //// insert scale between transform and shape
            SgPosTransform *tp = leftHand->topNode();
            left_scale = new SgScaleTransform();
            left_scale->setScale(0.8);//矢印の大きさを変える
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
                sgm->setAmbientIntensity(0.7);//ビームの太さ
                sgm->setTransparency(0.6);
            }
            { //// mesh
                Vector3 size(0.02, 0.02, AXIS_LENGTH);
                MeshGenerator mg;
                sph->setMesh(mg.generateBox(size));
            }
            sph->setName("right beam");
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
            right_switch->addChild(right_scale);
            tp->addChild(right_switch);
        }
    }

    if(!!vr_plugin) {//コントローラの情報取得
        vr_plugin->sigUpdateControllerState().connect(std::bind(&AssemblerVRProcess::updateControllerState, this,
                                                                std::placeholders::_1, std::placeholders::_2));
    }

    if(!!vr_plugin) {
        //vr_plugin->setProjectionMatrix(2.0);
        //vr_plugin->setEyeDifferenceScale(1/4.0);
    }
}

void AssemblerVRProcess::updateControllerState(const controllerState &left, const controllerState &right)//コントローラの状態更新
{
    setLeftCoords(left.coords);
    setRightCoords(right.coords);

    //// sample xxx
    if (!!as_manager) {
        robot_assembler::RASceneRobot *rb = as_manager->searchNearest(right.coords.pos, 0.1);
        if (!!rb) {
            as_manager->selectRobot(rb);
            as_manager->moveRobot(rb, right.coords);
        }

    }
    return;
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
