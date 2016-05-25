#include "DevClient.h"
#include "StationConfig.h"
#include "device_session.h"
#include "./dev_message/104/iec104_types.h"
#include "LocalConfig.h"
//#include <glog/logging.h>
//--------------------remark--------------------------------------//
//@author:liukun
//@data:2013-11-16
//@
//@增加汇鑫发射机数据管理器(该管理器可以连接1-2部发射机或993采集器与1部汇鑫天线控制器),本管理对象需区分该设备连接所
//关联的设备信息,自动区分无需连接的设备,自动装载需要连接的设备.
namespace hx_net
{
DevClient::DevClient(size_t io_service_pool_size/* =4 */)
    :io_service_pool_(io_service_pool_size)
    ,http_request_session_ptr_(new http_request_session(io_service_pool_.get_io_service(),false))
    ,http_report_session_ptr_(new http_request_session(io_service_pool_.get_io_service(),false))
{

}

DevClient::~DevClient()
{

    device_pool_.clear();
}

//连接所有设备
void DevClient::connect_all()
{
    string sLocalStationId = GetInst(LocalConfig).local_station_id();
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);

    vector<ModleInfo> &vModle = GetInst(StationConfig).get_Modle();
    vector<ModleInfo>::iterator modle_iter = vModle.begin();
    for(;modle_iter != vModle.end();modle_iter++)
    {
       if(device_pool_.find(DevKey(sLocalStationId,(*modle_iter).sModleNumber))==device_pool_.end())
        {
            session_ptr new_session(new device_session(io_service_pool_.get_io_service(),(*modle_iter),http_report_session_ptr_));
            device_pool_[DevKey(sLocalStationId,(*modle_iter).sModleNumber)]=new_session;
            new_session->init_session_config();
            new_session->connect();
            /*if((*modle_iter).iCommunicationMode==CON_MOD_NET) {
                if((*modle_iter).netMode.inet_type == NET_MOD_TCP){
                    new_session->connect((*modle_iter).netMode.strIp,(*modle_iter).netMode.iremote_port);
                }else if((*modle_iter).netMode.inet_type == NET_MOD_UDP){
                    new_session->udp_connect((*modle_iter).netMode.strIp,(*modle_iter).netMode.iremote_port);
                }else if((*modle_iter).netMode.inet_type == NET_MOD_SNMP){
                    new_session->agent_connect((*modle_iter).netMode.strIp,(*modle_iter).netMode.iremote_port);
                }
            }*/
        }
    }
}

void DevClient::disconnect_all()
{
    {
        boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
        std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
        for(;iter!=device_pool_.end();iter++)
            (*iter).second->disconnect();
    }
}

void DevClient::run()
{
    io_service_pool_.run();
}
void DevClient::stop()
{
    io_service_pool_.stop();
}

//获得设备连接
con_state DevClient::get_dev_net_state(string sStationId,string sDevid)
{
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter)
    {
        if(iter->first.stationId == sStationId)
        {
            if(iter->second->is_contain_dev(sDevid))
                return iter->second->get_con_state();
        }
    }
    return con_disconnected;
}
//获得设备运行状态
dev_run_state DevClient::get_dev_run_state(string sStationId,string sDevid)
{
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter)
    {
        if(iter->first.stationId == sStationId)
        {
            if(iter->second->is_contain_dev(sDevid))
                return iter->second->get_run_state(sDevid);
         }
    }

    return dev_unknown;
}
//获得设备运行状态
void DevClient::get_dev_alarm_state(string sStationId,string sDevid,map<int,map<int,CurItemAlarmInfo> >& cellAlarm)
{
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter)
    {
        if(iter->first.stationId == sStationId)
        {
            if(iter->second->is_contain_dev(sDevid))
                return iter->second->get_alarm_state(sDevid,cellAlarm);
        }
    }
    return ;
}

bool DevClient::dev_base_info(string sStationId,DevBaseInfo& devInfo,string sDevid)
{
    bool bRtValue = false;
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter){
        if(iter->first.stationId == sStationId){
            if(iter->second->is_contain_dev(sDevid)){
                iter->second->dev_base_info(devInfo,sDevid);
                bRtValue = true;
                return bRtValue;
            }
        }
    }
    return bRtValue;
}

e_ErrorCode DevClient::start_exec_task(string sDevId,string sUser,int cmdType)
{
    e_ErrorCode opr_rlt = EC_DEVICE_NOT_FOUND;
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter){
            if(iter->second->is_contain_dev(sDevId)){
                iter->second->start_exec_task(sDevId,sUser,opr_rlt,cmdType);
                return opr_rlt;
            }
    }
    return opr_rlt;
}
//通用命令执行
e_ErrorCode DevClient::excute_command(int cmdType,devCommdMsgPtr lpParam)
{
    e_ErrorCode opr_rlt = EC_DEVICE_NOT_FOUND;
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter){
            if(iter->second->is_contain_dev(lpParam->sdevid())){
                iter->second->excute_command(cmdType,lpParam,opr_rlt);
                return opr_rlt;
            }
    }
    return opr_rlt;
}

//更新运行图
e_ErrorCode DevClient::update_monitor_time(string sDevId,map<int,vector<Monitoring_Scheduler> >& monitorScheduler,
                                           vector<Command_Scheduler> &cmmdScheduler)
{
    e_ErrorCode opr_rlt = EC_DEVICE_NOT_FOUND;
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter){
        if(iter->second->is_contain_dev(sDevId)){
            iter->second->update_monitor_time(sDevId,monitorScheduler,cmmdScheduler);
            return opr_rlt;
        }
    }
    return opr_rlt;
}
//更新告警配置
e_ErrorCode DevClient::update_dev_alarm_config(string sDevId,DeviceInfo &devInfo)
{
    e_ErrorCode opr_rlt = EC_DEVICE_NOT_FOUND;
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();++iter){
        if(iter->second->is_contain_dev(sDevId)){
            iter->second->update_dev_alarm_config(sDevId,devInfo);
            return opr_rlt;
        }
    }
   return EC_OBJECT_NULL;
}
int DevClient::get_modle_online_count()
{
    boost::recursive_mutex::scoped_lock lock(device_pool_mutex_);
    int ncount=0;
    std::map<DevKey,session_ptr>::iterator iter = device_pool_.begin();
    for(;iter!=device_pool_.end();iter++)
    {
        if((*iter).second->dev_type()!=DEV_OTHER &&
                (*iter).second->dev_type()!=DEV_TRANSMITTER_AGENT)
            continue;
        if((*iter).second->is_connected())
            ++ncount;
    }
    return (ncount>=0)?ncount:-1;
}

//上报http消息
e_ErrorCode   DevClient::response_http_msg(string sUrl,string &sContent,string sRqstType)
{
   //http_request_session_ptr response_session_ptr(new http_request_session(io_service_pool_.get_io_service()));
    //http_request_session_ptr_->openUrl(sUrl,sContent,sRqstType);
    http_request_session_ptr_->putHttpMessage(sUrl,sContent);
    return EC_OK;
}

}
