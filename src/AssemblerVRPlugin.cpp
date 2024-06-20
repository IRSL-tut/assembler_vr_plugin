#include <fmt/format.h>
#define IRSL_DEBUG
#include "irsl_debug.h"
#include <cnoid/MessageView>
#include <cnoid/App>

#include "AssemblerVRPlugin.h"
#include "AssemblerVRProcess.h"

using namespace cnoid;

namespace {
AssemblerVRPlugin* instance_ = nullptr;
}

//// Impl
class AssemblerVRPlugin::Impl
{
public:
    Impl(AssemblerVRPlugin *_self);

    void initialize();
public:
    //
    AssemblerVRPlugin *self;
    AssemblerVRProcess *proc;
    std::ostream *os_;
};

//// >>>> Impl
AssemblerVRPlugin::Impl::Impl(AssemblerVRPlugin *_self) : self(_self), proc(nullptr), os_(nullptr)
{
}
void AssemblerVRPlugin::Impl::initialize()
{
    proc = new AssemblerVRProcess();
    os_ = &(MessageView::instance()->cout(false));
}


//// <<<< Impl

//// static methods
AssemblerVRPlugin* AssemblerVRPlugin::instance()
{
    return instance_;
}

//// methods
AssemblerVRPlugin::AssemblerVRPlugin()
    : Plugin("AssemblerVR")
{
    require("OpenVR");
    require("RobotAssembler");

    instance_ = this;
    impl = new Impl(this);
}
AssemblerVRPlugin::~AssemblerVRPlugin()
{
}

bool AssemblerVRPlugin::initialize()
{
    DEBUG_PRINT();

    App::sigExecutionStarted().connect( [this]() {
                                            impl->initialize();
                                        });
    return true;
}

bool AssemblerVRPlugin::finalize()
{
    DEBUG_PRINT();
    return true;
}

const char* AssemblerVRPlugin::description() const
{
    static std::string text =
        fmt::format("AssemblerVR Plugin Version {}\n", CNOID_FULL_VERSION_STRING) +
        "\n" +
        "Copyrigh (c) 2024 IRSL-tut Development Team.\n"
        "\n" +
        MITLicenseText() +
        "\n"  ;

    return text.c_str();
}

CNOID_IMPLEMENT_PLUGIN_ENTRY(AssemblerVRPlugin);
