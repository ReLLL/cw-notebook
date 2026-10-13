// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_adapter.h"
#include <core.h>
#include <cstdlib>
#include <cctype>
#if !CW_HOST_BROWN
#include <radio_interface.h>
#endif
namespace cw::host {
std::filesystem::path logFolder(){
    const char* home=std::getenv("HOME");
    return std::filesystem::path(home?home:".")/"Documents"/name/"CW Logs";
}
std::string mode(const std::string& receiver){
#if CW_HOST_BROWN
    auto it=core::moduleManager.instances.find(receiver);
    if(it!=core::moduleManager.instances.end()&&it->second.instance){
        try{return json::parse(it->second.instance->handleDebugCommand("get_demod","")).value("demod","");}
        catch(const json::exception&){}
    }
#else
    if(core::modComManager.interfaceExists(receiver)&&core::modComManager.getModuleName(receiver)=="radio"){
        int selected=-1;
        if(core::modComManager.callInterface(receiver,RADIO_IFACE_CMD_GET_MODE,nullptr,&selected)){
            switch(selected){
                case RADIO_IFACE_MODE_CW:return "CW";
                case RADIO_IFACE_MODE_USB:return "USB";
                case RADIO_IFACE_MODE_LSB:return "LSB";
                case RADIO_IFACE_MODE_DSB:return "DSB";
                case RADIO_IFACE_MODE_AM:return "AM";
                case RADIO_IFACE_MODE_NFM:return "NFM";
                case RADIO_IFACE_MODE_WFM:return "WFM";
                case RADIO_IFACE_MODE_RAW:return "RAW";
            }
        }
    }
#endif
    return {};
}
void useCW(const std::string& receiver){
#if CW_HOST_BROWN
    auto it=core::moduleManager.instances.find(receiver);
    if(it!=core::moduleManager.instances.end()&&it->second.instance)
        it->second.instance->handleDebugCommand("set_demod","CW");
#else
    if(core::modComManager.interfaceExists(receiver)&&core::modComManager.getModuleName(receiver)=="radio"){
        int selected=RADIO_IFACE_MODE_CW;
        core::modComManager.callInterface(receiver,RADIO_IFACE_CMD_SET_MODE,&selected,nullptr);
    }
#endif
}
std::string grid(const std::string& instance){
    core::configManager.acquire();
    const auto& config=core::configManager.conf;
    std::string result;
    if(brown){
        if(config.contains("operatorLocation")&&config["operatorLocation"].is_string())result=config["operatorLocation"].get<std::string>();
    }else if(config.contains("moduleInstances")&&config["moduleInstances"].contains(instance)){
        const auto& module=config["moduleInstances"][instance];
        if(module.is_object()&&module.contains("receiverGrid")&&module["receiverGrid"].is_string())result=module["receiverGrid"].get<std::string>();
    }
    core::configManager.release();
    return result;
}
bool setGrid(std::string value,const std::string& instance){
    if(brown)return false; // Brown owns its native location setting.
    for(char& c:value)c=char(std::toupper(static_cast<unsigned char>(c)));
    if(!value.empty()){
        if(value.size()!=4&&value.size()!=6&&value.size()!=8)return false;
        for(size_t i=0;i<value.size();++i){
            bool valid=i<2?value[i]>='A'&&value[i]<='R':
                i<4||i>=6?value[i]>='0'&&value[i]<='9':value[i]>='A'&&value[i]<='X';
            if(!valid)return false;
        }
    }
    // Upstream removes unrecognized top-level keys. Keep settings in this instance.
    core::configManager.acquire();core::configManager.conf["moduleInstances"][instance]["receiverGrid"]=value;
    core::configManager.release(true);return true;
}
}
