#ifndef __CNOID_ASSEMBLERVR_PLUGIN_H__
#define __CNOID_ASSEMBLERVR_PLUGIN_H__

#include <cnoid/Plugin>

#include "exportdecl.h"

////
namespace cnoid {


class CNOID_EXPORT AssemblerVRPlugin : public Plugin
{
public:
    AssemblerVRPlugin();
    ~AssemblerVRPlugin();

    static AssemblerVRPlugin* instance();

    virtual bool initialize() override;
    virtual bool finalize() override;
    virtual const char* description() const override;

private:
    class Impl;
    Impl *impl;
};

}

#endif
