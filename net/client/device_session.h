#ifndef  __DEVICE_SESSION_
#define __DEVICE_SESSION_
#pragma once

#include "../taskqueue.h"
#include "../message.h"
#include "device_message.h"
#include "../net_session.h"
#include "../../DataType.h"
#include "../../DataTypeDefine.h"
//#include "../../DevAgent.h"
#include "MsgHandleAgent.h"
#include "http_request_session.h"
using boost::asio::io_service;
using boost::asio::ip::tcp;

namespace hx_net
{
class device_session;
typedef boost::shared_ptr<device_session> dev_session_ptr;
typedef boost::weak_ptr<device_session>    dev_session_weak_ptr;
    class device_session:public net_session
	{
	public:
		device_session(boost::asio::io_service& io_service, 
            ModleInfo & modinfo,http_request_session_ptr &httpPtr);
		~device_session();
		Dev_Type dev_type(){return DEV_OTHER;}
        //初始化设备配置
        void init_session_config();
        //设备基本信息
		void dev_base_info(DevBaseInfo& devInfo,string iId="local");
		//该连接是否包含此设备id
		bool is_contain_dev(string sDevId);
		//开始连接
		void connect(std::string hostname,unsigned short port,bool bReconnect=false);
		//udp连接
		void udp_connect(std::string hostname,unsigned short port);
		//断开连接
		void disconnect();
		//是否已建立连接
		bool is_connected(string sDevId="");
		//是否正在连接
		bool is_connecting();
		//是否已断开
		bool is_disconnected(string sDevId="");
		//获得协议转换器连接状态
		con_state       get_con_state();
		//获得设备运行状态（默认连接正常则运行正常）
        dev_run_state   get_run_state(string sDevId);
		//获得报警状态
        void get_alarm_state(string sDevId,map<int,map<int,CurItemAlarmInfo> >& cellAlarm);
		//执行通用指令
		bool excute_command(int cmdType,devCommdMsgPtr lpParam,e_ErrorCode &opResult);
		//清除所有报警标志
		void clear_all_alarm();
		//清除单设备报警
		void clear_dev_alarm(string sDevId);
		//发送消息
		bool sendRawMessage(unsigned char * data_,int nDatalen);
        //启动定时控制
        void start_task_schedules_timer();
        //发送命令到设备
        void send_cmd_to_dev(string sDevId,int cmdType,int childId);
	protected:
		void start_read_head(int msgLen);//开始接收头
		void start_read_body(int msgLen);//开始接收体
		void start_read(int msgLen);
		void start_query_timer(unsigned long nSeconds=2000);
		void start_connect_timer(unsigned long nSeconds=3);
		void start_timeout_timer(unsigned long nSeconds=10);
		void set_con_state(con_state curState);
		void start_write(unsigned char* commStr,int commLen);
		void connect_timeout(const boost::system::error_code& error);
		void connect_time_event(const boost::system::error_code& error);
		void query_send_time_event(const boost::system::error_code& error);
		void  handler_data(string sDevId,DevMonitorDataPtr curDataPtr);
		void set_stop(bool bStop=true);
		bool is_stop();
        void close_all();
		//判断监测量是否报警
		void check_alarm_state(string sDevId,DevMonitorDataPtr curDataPtr,bool bMonitor);

        void save_monitor_record(string sDevId,DevMonitorDataPtr curDataPtrconst,const map<int,DeviceMonitorItem> &mapMonitorItem);

		bool is_need_save_data(string sDevId);

		string next_dev_id();

		//提交任务
		void task_count_increase();
		//任务递减
		void task_count_decrease();
		//任务数
		int  task_count();
		//等待任务结束
		void wait_task_end();
		//是否在监测时间段
		bool is_monitor_time(string sDevId);

        void sendSmsToUsers(int nLevel,string &sContent);

        bool  excute_general_command(int cmdType,devCommdMsgPtr lpParam,e_ErrorCode &opResult);


        //-------2016-3-30------------------------//
        void  parse_item_alarm(string devId,float fValue,DeviceMonitorItem &ItemInfo);
        void  record_alarm_and_notify(string &devId,float fValue,const float &fLimitValue,bool bMod,
                                        DeviceMonitorItem &ItemInfo,CurItemAlarmInfo &curAlarm);

	public:	
		void handle_connected(const boost::system::error_code& error);
		void handle_read_head(const boost::system::error_code& error, size_t bytes_transferred);//通用消息头（分消息head，body）
		void handle_read_body(const boost::system::error_code& error, size_t bytes_transferred);//通用消息体
        void handle_udp_read(const boost::system::error_code& error,size_t bytes_transferred);//udp接收回调
		void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        void schedules_task_time_out(const boost::system::error_code& error);
		virtual void handle_write(const boost::system::error_code& error,size_t bytes_transferred);
       //开始执行任务
        bool start_exec_task(string sDevId,string sUser,e_ErrorCode &opResult,int cmdType);
        void notify_client(string sDevId,string devName,string user,int cmdType, tm *pCurTime,int eResult);
        void set_opr_state(string sdevId,dev_opr_state curState);
        dev_opr_state get_opr_state(string sdevId);
	private:
		boost::mutex					stop_mutex_;
		bool								    stop_flag_;
		tcp::resolver                      resolver_;	
		udp::resolver                    uresolver_;

		tcp::endpoint                   endpoint_;
		udp::endpoint                   uendpoint_;
		boost::recursive_mutex          con_state_mutex_;
		con_state                       othdev_con_state_;
		othdevMsgPtr                    receive_msg_ptr_;
		boost::asio::deadline_timer     connect_timer_;//连接定时器
		boost::asio::deadline_timer     timeout_timer_;//连接超时定时器
		boost::asio::deadline_timer     query_timer_;//查询定时器   
        boost::asio::deadline_timer     schedules_task_timer_;//控制任务执行定时器

        size_t                          query_timeout_count_;//查询命令执行超时次数
		size_t                          cur_msg_q_id_;//当前发送的消息序号

		boost::recursive_mutex          data_deal_mutex;
		boost::recursive_mutex          alarm_state_mutex;
        //devid<itemid<iLimittype,info> > >
        map<string ,map<int,map<int,CurItemAlarmInfo> > > mapItemAlarm;//设备监控量告警信息
        CurItemAlarmInfo  netAlarm;//通讯异常告警
		map<string,time_t>                               tmLastSaveTime;
        map<string,pair<CommandAttrPtr,HMsgHandlePtr> >   dev_agent_and_com;//add by lk 2013-11-26
		string                                           cur_dev_id_;//当前查询设备id

        ModleInfo                          &modleInfos;
        boost::mutex                        task_mutex_;
        int										   task_count_;
        boost::condition                    task_end_conditon_;

        map<string,pDevicePropertyExPtr>       run_config_ptr;//moxa下设备配置
        pMoxaPropertyExPtr              moxa_config_ptr;//moxa配置

#ifdef USE_STRAND
		io_service::strand              strand_;   //消息头与消息体同步
#endif

        http_request_session_ptr   &http_ptr_;

        boost::recursive_mutex                    opr_state_mutex_;
        map<string,dev_opr_state>                   dev_opr_state_;//设备控制命令发送状态
        boost::asio::io_service&          io_service_;
        //map<string,string>				   cur_opr_user_;//当前命令发起用户
        //map<string,int>                    cur_task_type_;//当前任务类型
        //map<string,time_t>               cur_task_start_time_;//当前任务提交开始时间
	};
}
#endif
