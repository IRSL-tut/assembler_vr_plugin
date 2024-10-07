#include "AssemblerVRProcess.h"

#include <cnoid/RootItem>
#include <cnoid/MeshGenerator>

//// pick
#include <cnoid/SceneView>
#include <cnoid/SceneWidget>
#include <cnoid/SceneRenderer>
#include <cnoid/GLSLSceneRenderer>

using namespace cnoid;

static robot_assembler::RASceneBase *pick_object(coordinates &cam_coords)
{
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT;
    cam_coords.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    sw->makeCurrent();
    sr->pick(sw->width()/2, sw->height()/2);
    sw->doneCurrent();

    sw->builtinCameraTransform()->setPosition(orgT);

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!!glsr) {
        //glsr->pickedNodePoint();
        const SgNodePath &np = glsr->pickedNodePath();
        const Vector3    &pt = glsr->pickedPoint();
        std::cout << "point : " << pt.x() << ", " << pt.y() << ", " << pt.z() << std::endl;
        std::cout << "NodePath : size : " << np.size() << std::endl;
        if (np.size() == 0) {
            return nullptr;
        }
        for(auto n = np.begin(); n != np.end(); n++) {
            SgNode *ptr = *n;
            //std::cout << "  name:" << (*n)->name();
            //std::cout << ", cls:" << (*n)->className() << std::endl;
            robot_assembler::RASceneBase *res = dynamic_cast<robot_assembler::RASceneBase *>(ptr);
            if (!!res) {
                return res;
            }
        }
    }
    return nullptr;
}

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
            //// register Item
            rightHand = static_cast<SceneItem *>(p);

            //// making beam
#define AXIS_LENGTH 30.0
            right_switch = new SgSwitchableGroup();
            SgPosTransform *trs = new SgPosTransform();
            SgShape *sph = new SgShape();
            { //// material
                SgMaterial *sgm = sph->getOrCreateMaterial();
                Vector3f diff(0., 1., 1.); //// color
                sgm->setDiffuseColor(diff);
                sgm->setAmbientIntensity(0.7);
                sgm->setTransparency(0.6);
            }
            { //// mesh
                Vector3 size(AXIS_LENGTH, 0.02, 0.02);
                MeshGenerator mg;
                sph->setMesh(mg.generateBox(size));
            }
            trs->setTranslation(Vector3(AXIS_LENGTH*0.5, 0, 0));
            trs->addChild(sph);
            right_switch->addChild(trs);
            rightHand->topNode()->addChild(right_switch);
            right_switch->setTurnedOn(true);
            rightHand->topNode()->notifyUpdate(SgUpdate::Modified);
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
