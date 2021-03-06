#ifndef DEV_CLIENT_H
#define DEV_CLIENT_H

#pragma once

#include "../io_service_pool.h"
#include "../../qnotifyhandler.h"
#include "http_request_session.h"
using boost::asio::ip::tcp;

namespace hx_net
{
	class DevClient
	{
	public:
		struct DevKey
		{
			DevKey(string _stId,string _devId)
			{
				stationId =  _stId;
				devId = _devId;
			}
			bool operator == (const DevKey &other)const
			{
				if(other.stationId == stationId
					&& other.devId == devId)
					return true;
				return false;
			}

			bool operator < (const DevKey &other)const
			{
				if( stationId<other.stationId )
					return true;
				else if(stationId==other.stationId)
				{
					return devId<other.devId;
				}
				return false;
			}

			string stationId;
			string devId;
		};

		//初始化一个客户端，该客户端维护一个任务队列，并创建一个io_service
        DevClient(size_t io_service_pool_size=2);
		~DevClient();
	public:
		void run();//启动io_service pool
		void stop();
		void connect_all();
		void disconnect_all();
		
		//通用命令执行
		e_ErrorCode excute_command(int cmdType,devCommdMsgPtr lpParam);

        e_ErrorCode start_exec_task(string sDevId,string sUser,int cmdType);
		//获取转换模块在线数
		int get_modle_online_count();

		//获得设备连接
		con_state get_dev_net_state(string sStationId,string sDevid);
		//获得设备运行状态
		dev_run_state get_dev_run_state(string sStationId,string sDevid);
        //获得设备告警状态
        void get_dev_alarm_state(string sStationId,string sDevid,map<int,map<int,CurItemAlarmInfo> >& cellAlarm);
		//获得设备类型
		bool dev_base_info(string sStationId,DevBaseInfo& devInfo,string sDevid="local");

        //更新运行图
        e_ErrorCode update_monitor_time(string sDevId,map<int,vector<Monitoring_Scheduler> >& monitorScheduler,
                                                   vector<Command_Scheduler> &cmmdScheduler);
        //更新告警配置
        e_ErrorCode update_dev_alarm_config(string sDevId,DeviceInfo &devInfo);

        //上报http消息
        e_ErrorCode   response_http_msg(string sUrl,string &sContent,string sRqstType="POST");
	private:
		boost::recursive_mutex         device_pool_mutex_;
		std::map<DevKey,session_ptr>   device_pool_;
		io_service_pool                io_service_pool_;

        http_request_session_ptr      http_request_session_ptr_;
        http_request_session_ptr      http_report_session_ptr_;
	};
}

#endif
