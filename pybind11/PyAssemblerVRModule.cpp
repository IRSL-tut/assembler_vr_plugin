/**
   @author YoheiKakiuchi
*/

#include <cnoid/PyUtil>
#include <cnoid/PyBase>

#include <cnoid/SceneView>
#include <cnoid/SceneWidget>
#include <cnoid/SceneRenderer>
#include <cnoid/GLSLSceneRenderer>

#include <iostream>

#include "../../robot_assembler_plugin/src/irsl_choreonoid/Coordinates.h"

#include "../src/AssemblerVRPlugin.h"
#include "../src/AssemblerVRProcess.h"

using namespace cnoid;
namespace py = pybind11;

bool pick(int x, int y)
{
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    sw->makeCurrent();
    sr->pick(x, y);
    sw->doneCurrent();

    GLSLSceneRenderer *glsr = static_cast<GLSLSceneRenderer *>(sr);
    if (!!glsr) {
        //glsr->pickedNodePoint();
        const SgNodePath &np = glsr->pickedNodePath();
        const Vector3    &pt = glsr->pickedPoint();
        std::cout << "point : " << pt.x() << ", " << pt.y() << ", " << pt.z() << std::endl;
        std::cout << "NodePath : size : " << np.size() << std::endl;
        if (np.size() == 0) {
            return false;
        }
        for(auto n = np.begin(); n != np.end(); n++) {
            std::cout << "  name:" << (*n)->name();
            std::cout << ", cls:" << (*n)->className() << std::endl;
        }
    }
    return true;
}

bool pick_cam(int x, int y, coordinates &c)
{
    SceneView     *sv = SceneView::instance();
    SceneWidget   *sw = sv->sceneWidget();
    SceneRenderer *sr = sv->renderer();

    Isometry3 orgT = sw->builtinCameraTransform()->T();
    //double orgFov   = sw->builtinPerspectiveCamera()->fieldOfView();

    Isometry3 newT; c.toPosition(newT);
    sw->builtinCameraTransform()->setPosition(newT);

    sw->makeCurrent();
    sr->pick(x, y);
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
            return false;
        }
        for(auto n = np.begin(); n != np.end(); n++) {
            std::cout << "  name:" << (*n)->name();
            std::cout << ", cls:" << (*n)->className() << std::endl;
        }
    }
    return true;
}
PYBIND11_MODULE(AssemblerVR, m)
{
    m.doc() = "python-binding for AssemblerVR";

    py::module::import("cnoid.Util");
    py::module::import("cnoid.Base");
    //py::module::import("cnoid.IRSLCoords");

    m.def("pick", &pick);
    m.def("pick_cam", &pick_cam);

    py::class_<AssemblerVRPlugin> plugin(m, "AssemblerVRPlugin");
    plugin
    .def_property_readonly_static("instance", [](py::object){ return AssemblerVRPlugin::instance(); })
    ;

    py::class_<AssemblerVRProcess> proc(m, "AssemblerVRProcess");
    proc
    .def_property_readonly_static("instance", [](py::object){
        return AssemblerVRPlugin::instance()->getProcess();
    })
    .def("pick_object", &AssemblerVRProcess::pick_object)
    .def("setLeftCoords", &AssemblerVRProcess::setLeftCoords)
    .def("setRightCoords", &AssemblerVRProcess::setRightCoords)
    .def("setProjectionMatrix", [](AssemblerVRProcess &self, double scale) {
        if (!!(self.vr_plugin)) {
            self.vr_plugin->setProjectionMatrix(scale);
        }
    })
    .def("setEyeDifferenceScale", [](AssemblerVRProcess &self, double scale) {
        if (!!(self.vr_plugin)) {
            self.vr_plugin->setEyeDifferenceScale(scale);
        }
    })
    ;
}
