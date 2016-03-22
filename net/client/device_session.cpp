#include "device_session.h"
#include <boost/thread/detail/singleton.hpp>
#include "../../ErrorCode.h"
#include "../SvcMgr.h"
#include "../../LocalConfig.h"
#include "../../StationConfig.h"
//#include "../sms/SmsTraffic.h"

namespace net
{
	device_session::device_session(boost::asio::io_service& io_service, 
		TaskQueue<msgPointer>& taskwork,ModleInfo & modinfo)
		:session(io_service)
		,taskwork_(taskwork)
#ifdef USE_CLIENT_STRAND
		, strand_(io_service)
#endif 
		,resolver_(io_service)
		,uresolver_(io_service)
		,query_timer_(io_service)
		,connect_timer_(io_service)
		,timeout_timer_(io_service)
		,receive_msg_ptr_(new othdev_message(2048))
		,othdev_con_state_(con_disconnected)
		,modleInfos(modinfo)
		,cur_msg_q_id_(0)
		,command_timeout_count_(0)
		,stop_flag_(false)
		,task_count_(0)
	{
		//获得网络协议转换器相关配置
		moxa_config_ptr = GetInst(LocalConfig).moxa_property_ex(modleInfos.sModleNumber);
        map<string,DeviceInfo>::iterator iter = modleInfos.mapDevInfo.begin();
        for(;iter!=modleInfos.mapDevInfo.end();++iter)
		{	
			map<int,double> itemRadio;
            map<int,DeviceMonitorItem>::iterator iterItem = iter->second.map_MonitorItem.begin();
            for(;iterItem!=iter->second.map_MonitorItem.end();++iterItem)
			{
				itemRadio.insert(pair<int,double>(iterItem->first,iterItem->second.dRatio));
			}

			//------------------ new code -----------------------------------------//
			HMsgHandlePtr pars_agent = HMsgHandlePtr(new MsgHandleAgent(this,io_service));
			boost::shared_ptr<CommandAttribute> tmpCommand(new CommandAttribute);
			pars_agent->Init((*iter).second.nDevProtocol,(*iter).second.nSubProtocol,(*iter).second.nDevAddr,itemRadio);


			pTransmitterPropertyExPtr dev_config_ptr = GetInst(LocalConfig).transmitter_property_ex((*iter).first);
					
            //pars_agent->DevAgent()->HxSetAttribute(dev_config_ptr->u0_range_value,dev_config_ptr->i0_range_value,
            //									   dev_config_ptr->ubb_ratio_value,dev_config_ptr->ibb_ratio_value);
			
			//保存moxa下的设备自定义配置信息
			run_config_ptr[(*iter).first]=dev_config_ptr;
            //pars_agent->DevAgent()->HxGetAllCmd(*tmpCommand);
// 
// 			//增加读配置文件获取命令回复长度...
// 			if(dev_config_ptr->cmd_ack_length_by_id.size()>0)
// 			{
// 				for(int nCmdCount = 0;nCmdCount<tmpCommand->queryComm.size();++nCmdCount)
// 					tmpCommand->queryComm[nCmdCount].ackLen = dev_config_ptr->cmd_ack_length_by_id[nCmdCount];
// 			}
			CommandUnit  commUnit;
			commUnit.ackLen=2;

			tmpCommand->queryComm.push_back(commUnit);//测试代码---------------delete later
			dev_agent_and_com[iter->first]=pair<CommandAttrPtr,HMsgHandlePtr>(tmpCommand,pars_agent);

			//报警项初始化
            map<int,std::pair<int,unsigned int> > devAlarmItem;
			mapItemAlarmRecord.insert(std::make_pair(iter->first,devAlarmItem));
			//定时数据保存时间初始化
			tmLastSaveTime.insert(std::make_pair(iter->first,time(0)));
		}
	  cur_dev_id_ = dev_agent_and_com.begin()->first;


	}

	device_session::~device_session()
	{

	}


	void device_session::dev_base_info(DevBaseInfo& devInfo,string iId)
	{

		if(modleInfos.mapDevInfo.find(iId)!=modleInfos.mapDevInfo.end())
		{
			devInfo.mapMonitorItem = modleInfos.mapDevInfo[iId].mapMonitorItem;
			devInfo.nDevType = modleInfos.mapDevInfo[iId].nDevType;
			devInfo.sDevName = modleInfos.mapDevInfo[iId].sDevName;
			devInfo.sDevNum = modleInfos.mapDevInfo[iId].sDevNum;
			devInfo.sDevNum = modleInfos.mapDevInfo[iId].sStationNumber;
			devInfo.nCommType = modleInfos.mapDevInfo[iId].nCommType;//主被动连接
			devInfo.nConnectType =modleInfos.mapDevInfo[iId].nConnectType;//连接方式0：tcp
		}
	}

	//是否包含该id
	bool device_session::is_contain_dev(string sDevId)
	{
		if(modleInfos.mapDevInfo.find(sDevId) == modleInfos.mapDevInfo.end())
			return false;
		return true;
	}

	string device_session::next_dev_id()
	{ 
        map<string,pair<CommandAttrPtr,HMsgHandlePtr> >::iterator iter = dev_agent_and_com.find(cur_dev_id_);
		++iter;
		if(iter==dev_agent_and_com.end())
			iter = dev_agent_and_com.begin();
		return iter->first;
	}

	//获得连接状态
	con_state device_session::get_con_state()
	{
		boost::recursive_mutex::scoped_lock llock(con_state_mutex_);
		return othdev_con_state_;
	}
	//获得运行状态
	dev_run_state device_session::get_run_state()
	{
		
		if(get_con_state()==con_connected)
			return dev_running;
		else
			return dev_unknown;
	}

	void device_session::set_con_state(con_state curState)
	{
		boost::recursive_mutex::scoped_lock llock(con_state_mutex_);
		if(othdev_con_state_!=curState)
		{
			othdev_con_state_ = curState;
			//GetInst(SvcMgr).get_notify()->OnConnected(this->modleInfos.sModleNumber,othdev_con_state_);
			map<string,DevParamerInfo>::iterator iter = modleInfos.mapDevInfo.begin();
			for(;iter!=modleInfos.mapDevInfo.end();++iter)
			{
				//广播设备状态到在线客户端
				send_net_state_message(modleInfos.sStationNumber,(*iter).first
										,(*iter).second.sDevName,(e_DevType)((*iter).second.nDevType)
										,othdev_con_state_);
				GetInst(SvcMgr).get_notify()->OnConnected((*iter).first,othdev_con_state_);
			}

		}
	}

	//获得发射机报警状态
    void device_session::get_alarm_state(string sDevId,map<int,std::pair<int,tm> >& cellAlarm)
	{
		boost::recursive_mutex::scoped_lock lock(alarm_state_mutex);
		if(mapItemAlarmStartTime.find(sDevId)!=mapItemAlarmStartTime.end())
			cellAlarm = mapItemAlarmStartTime[sDevId];
	}


	void device_session::connect(std::string hostname,unsigned short port,bool bReconnect)
	{	
		if(is_connected())
		{
			//std::string id = str(boost::format("%1%:%2%")%hostname%port);
			//std::cout<<id<<"is connected!!"<<std::endl;
			return;
		}
		set_stop(false);

		boost::system::error_code ec;     
		tcp::resolver::query query(hostname, boost::lexical_cast<std::string, unsigned short>(port));     
		tcp::resolver::iterator iter = resolver_.resolve(query, ec);  
		if(iter != tcp::resolver::iterator())
		{
			//正在连接
			set_con_state(con_connecting);
			endpoint_ = (*iter).endpoint();
			socket().async_connect(endpoint_,boost::bind(&device_session::handle_connected,
				this,boost::asio::placeholders::error));
			start_timeout_timer();//启动超时重连定时器
		}
		else
		{
			//出错。。。
		}
	}

	//udp连接
	void device_session::udp_connect(std::string hostname,unsigned short port)
	{
		set_tcp(false);
		boost::system::error_code ec; 
		udp::resolver::query query(hostname,boost::lexical_cast<std::string, unsigned short>(port));
		udp::resolver::iterator iter = uresolver_.resolve(query,ec);
		if(iter!=udp::resolver::iterator())
		{
			uendpoint_ = (*iter).endpoint();
			usocket().open(udp::v4());
			const udp::endpoint local_endpoint = udp::endpoint(udp::v4(),port);
			//local_endpoint.
			usocket().bind(local_endpoint);
			//set_con_state(con_connected);
			boost::system::error_code err= boost::system::error_code();
			handle_connected(err);
		}
	}

	//启动超时定时器
	void device_session::start_timeout_timer(unsigned long nSeconds)
	{
		timeout_timer_.expires_from_now(boost::posix_time::seconds(nSeconds));
		timeout_timer_.async_wait(boost::bind(&device_session::connect_timeout,
			this,boost::asio::placeholders::error));
	}

	//连接超时回调
	void device_session::connect_timeout(const boost::system::error_code& error)
	{
		if(is_stop())
			return;
		//只有当前状态是正在连接才执行超时。。。
		if(!is_connecting())
			return;
		//超时了
		if(error!= boost::asio::error::operation_aborted)
			start_connect_timer();//启动重连尝试
	}

	void device_session::set_stop(bool bStop)
	{
		boost::mutex::scoped_lock lock(stop_mutex_);
		stop_flag_=bStop;
	}

	bool device_session::is_stop()
	{
		boost::mutex::scoped_lock lock(stop_mutex_);
		return stop_flag_;
	}
	//是否已经建立连接
	bool device_session::is_connected(string sDevId)
	{
		return (get_con_state()==con_connected)?true:false;
	}
	//是否正在连接
	bool  device_session::is_connecting()
	{
		return (get_con_state()==con_connecting)?true:false;
	}
	//是否已经断开连接
	bool device_session::is_disconnected(string sDevId)
	{
		return (get_con_state()==con_disconnected)?true:false;
	}


	void device_session::start_connect_timer(unsigned long nSeconds)
	{
		set_con_state(con_connecting);//设置正在重连标志
		connect_timer_.expires_from_now(boost::posix_time::seconds(nSeconds));
		connect_timer_.async_wait(boost::bind(&device_session::connect_time_event,
			this,boost::asio::placeholders::error));
	}

	//连接超时
	void device_session::connect_time_event(const boost::system::error_code& error)
	{
		//只有当前状态是正在连接才执行超时。。。
		if(!is_connecting())
			return;
		if(error!= boost::asio::error::operation_aborted)
		{
			if(is_tcp())
			{
				socket().async_connect(endpoint_,boost::bind(&device_session::handle_connected,
					this,boost::asio::placeholders::error)); 
			}else{
				usocket().open(udp::v4());
				const udp::endpoint local_endpoint = udp::endpoint(udp::v4(),modleInfos.nModPort);
				usocket().bind(local_endpoint);
				boost::system::error_code err= boost::system::error_code();
				handle_connected(err);
			}


			//std::cout<<"reconnect request..."<<std::endl;
			start_timeout_timer();//启动超时定时器
		}
	}


	void device_session::disconnect()
	{
		set_stop();
		close_all();
		clear_all_alarm();//当停止服务时，清除所有报警
	}

	void device_session::start_read(int msgLen)
	{
		if(msgLen > receive_msg_ptr_->space())
		{
			cout<<"data overflow !!!!"<<endl;
			return;
		}
		boost::asio::async_read(socket(),boost::asio::buffer(receive_msg_ptr_->w_ptr(),msgLen),
#ifdef USE_STRAND
			strand_.wrap(
#endif
			boost::bind(&device_session::handle_read,this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
			)
#endif
			);
	}

	void device_session::start_query_timer(unsigned long nSeconds/* =3 */)
	{
		query_timer_.expires_from_now(boost::posix_time::millisec(nSeconds));
		query_timer_.async_wait(
#ifdef USE_STRAND
			strand_.wrap(
#endif
			boost::bind(&device_session::query_send_time_event,this,boost::asio::placeholders::error)
#ifdef USE_STRAND
			)
#endif
			);
	}

	void  device_session::query_send_time_event(const boost::system::error_code& error)
	{
		if(!is_connected())
			return;
		if(error!= boost::asio::error::operation_aborted)
		{
			if(command_timeout_count_<moxa_config_ptr->query_timeout_count)
			{
				++command_timeout_count_;
				if(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandLen>0){
					start_write(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandId,
						dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandLen 
						);
				}
			}
			else{
				command_timeout_count_ = 0;
				cur_msg_q_id_ = 0;
				//发送清零数据给客户端,该设备可能连接异常
				DevMonitorDataPtr curData_ptr(new Data);
				send_monitor_data_message(modleInfos.sStationNumber,cur_dev_id_,(e_DevType)modleInfos.mapDevInfo[cur_dev_id_].nDevType
					,curData_ptr,modleInfos.mapDevInfo[cur_dev_id_].mapMonitorItem);
				
				cur_dev_id_ = next_dev_id();
				if(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandLen>0){
					start_write(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandId,
						dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandLen 
						);
				}
			}
			start_query_timer(moxa_config_ptr->query_interval);
		}
	}

	//发送消息
	bool device_session::sendRawMessage(unsigned char * data_,int nDatalen)
	{
		char dataPrint[100];
		memset(dataPrint,0,100);
		memcpy(dataPrint,data_,nDatalen);
		start_write(data_,nDatalen);
		return true;
	}

	void device_session::start_write(unsigned char* commStr,int commLen)
	{
		if(is_tcp())
		{
			boost::asio::async_write(
				socket(),
				boost::asio::buffer(commStr,commLen),
#ifdef USE_STRAND
				strand_.wrap(
#endif
				boost::bind(&session::handle_write,shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
				)
#endif
				);
		}


		else
		{
			usocket().async_send_to(boost::asio::buffer(commStr,commLen),uendpoint_,
#ifdef USE_STRAND
				strand_.wrap(
#endif
				boost::bind(&session::handle_write,shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
				)
#endif
				);
		}
	}

	//等待任务结束
	void device_session::wait_task_end()
	{
		boost::mutex::scoped_lock lock(task_mutex_);
		while(task_count_>0)
		{
			task_end_conditon_.wait(task_mutex_);
		}
	}

	//提交任务
	void device_session::task_count_increase()
	{
		boost::mutex::scoped_lock lock(task_mutex_);
		++task_count_;
	}
	//任务递减
	void device_session::task_count_decrease()
	{
		boost::mutex::scoped_lock lock(task_mutex_);
		--task_count_;
		task_end_conditon_.notify_all();
	}

	int device_session::task_count()
	{
		boost::mutex::scoped_lock lock(task_mutex_);
		return task_count_;
	}
	void device_session::close_all()
	{
		set_con_state(con_disconnected);
		connect_timer_.cancel();
		timeout_timer_.cancel();
		close_i();   //关闭socket
		//等待任务结束，任务里面，必须放在运行状态检测之前
		//状态检测会
		wait_task_end();
		cur_msg_q_id_         =0;//当前命令id
		command_timeout_count_=0;//命令发送超时次数清零

		clear_all_alarm();

	}


	//判断是否保存当前记录
	void device_session::save_monitor_record(string sDevId,DevMonitorDataPtr curDataPtr)
	{
		if(is_need_save_data(sDevId))
		{
            GetInst(DbManager).SaveOthDevMonitoringData(modleInfos.sStationNumber,
					sDevId,modleInfos.mapDevInfo[sDevId].mapMonitorItem,curDataPtr);
		}

	}

	//判断当前时间是否需要保存监控数据
	bool device_session::is_need_save_data(string sDevId)
	{
		time_t tmCurTime;
		time(&tmCurTime);
		double ninterval = difftime(tmCurTime,tmLastSaveTime[sDevId]);
		if(ninterval<run_config_ptr[sDevId]->data_save_interval)//间隔保存时间 need amend;
			return false;
		tmLastSaveTime[sDevId] = tmCurTime;
		return true;
	}


	//是否在监测时间段
	bool device_session::is_monitor_time(string sDevId)
	{
		time_t curTime = time(0);
		tm *pCurTime = localtime(&curTime);
		vector<MonitorScheduler>::iterator witer = modleInfos.mapDevInfo[sDevId].vecMonitorScheduler.begin();
		for(;witer!=modleInfos.mapDevInfo[sDevId].vecMonitorScheduler.end();++witer)
		{
			if(((*witer).nMonitorDay+1)%7 == pCurTime->tm_wday && 
				(*witer).bEnable == true){
				unsigned long tmStart = (*witer).tmStartTime.tm_hour*3600+
					(*witer).tmStartTime.tm_min*60+
					(*witer).tmStartTime.tm_sec;
				unsigned long tmEnd   = (*witer).tmCloseTime.tm_hour*3600+
					(*witer).tmCloseTime.tm_min*60+
					(*witer).tmCloseTime.tm_sec;
				unsigned long tmCur   =  pCurTime->tm_hour*3600+pCurTime->tm_min*60+pCurTime->tm_sec;

				if(tmCur>=tmStart && tmCur<=tmEnd)
					return true;
			}
		}

		return false;
	}

	void device_session::handler_data(string sDevId,DevMonitorDataPtr curDataPtr)
	{
		boost::recursive_mutex::scoped_lock lock(data_deal_mutex);

		bool bIsMonitorTime = is_monitor_time(sDevId);
		//打包发送客户端
		send_monitor_data_message(modleInfos.sStationNumber,sDevId,(e_DevType)modleInfos.mapDevInfo[sDevId].nDevType
								  ,curDataPtr,modleInfos.mapDevInfo[sDevId].mapMonitorItem);
		//检测当前报警状态
		if(bIsMonitorTime)
			check_alarm_state(sDevId,curDataPtr,bIsMonitorTime);
		else
			clear_dev_alarm(sDevId);
		//如果在监测时间段则保存当前记录
		if(bIsMonitorTime)
			save_monitor_record(sDevId,curDataPtr);
		//任务数递减
		task_count_decrease();
		return;
	}

	void device_session::clear_dev_alarm(string sDevId)
	{
		boost::recursive_mutex::scoped_lock lock(alarm_state_mutex);
		if(mapItemAlarmStartTime.size()>0)
		{

			map<int,DevParamerMonitorItem>::iterator iter = modleInfos.mapDevInfo[sDevId].mapMonitorItem.begin();
			for(;iter!=modleInfos.mapDevInfo[sDevId].mapMonitorItem.end();++iter)
			{
				if((*iter).second.bAlarmEnable!=true)
					continue;
				if(mapItemAlarmRecord[sDevId].find((*iter).first)==mapItemAlarmRecord[sDevId].end())
					continue;
				time_t curTime = time(0);
				tm *ltime = localtime(&curTime);//恢复时间
                GetInst(DbManager).UpdateTransmitterAlarmEndTime(ltime,
					mapItemAlarmStartTime[sDevId][(*iter).first].second,modleInfos.mapDevInfo[sDevId].sStationNumber,
					modleInfos.mapDevInfo[sDevId].sDevNum,(*iter).first);

				//移除该报警项
				mapItemAlarmStartTime[sDevId].erase((*iter).first);

				//通知客户端(报警恢复通知客户端)
				//广播设备监控量报警到客户端
				std::string alramTm = str(boost::format("%1%/%2%/%3% %4%:%5%:%6%")
														%(ltime->tm_year+1900)
														%(ltime->tm_mon+1)
														%ltime->tm_mday
														%ltime->tm_hour
														%ltime->tm_min
														%ltime->tm_sec);
				send_alarm_state_message(modleInfos.sStationNumber,sDevId
					,modleInfos.mapDevInfo[sDevId].sDevName,(*iter).second.nMonitoringIndex
					,(*iter).second.sMonitoringName
					,(e_DevType)modleInfos.mapDevInfo[sDevId].nDevType
					,resume_normal,alramTm,mapItemAlarmStartTime[sDevId].size());
			}
		}

		if(mapItemAlarmRecord.size()>0)
		{
            map<string,map<int,std::pair<int,unsigned int> > >::iterator iter = mapItemAlarmRecord.find(sDevId);
			if(iter!=mapItemAlarmRecord.end())
				mapItemAlarmRecord.erase(iter);   //报警记录清除
		}

	}

	void device_session::handle_connected(const boost::system::error_code& error)
	{
		if(is_stop())
			return;
		timeout_timer_.cancel();//关闭重连超时定时器

		if(!error)
		{
			//std::cout << "Othdev_session----->connect success!!"<<endl;
			set_con_state(con_connected);
			//开始启动接收第一条查询指令的回复数据头
			receive_msg_ptr_->reset();	
			if(dev_agent_and_com[cur_dev_id_].second->IsStandardCommand()==true)
			{
				//dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].ackLen=6;//测试代码------------------delete later
				start_read_head(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].ackLen);//
			}
			else
				start_read(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].ackLen);

			if(is_tcp())
			{
				if(dev_agent_and_com[cur_dev_id_].second->is_auto_run()==false)
					start_query_timer(moxa_config_ptr->query_interval);
				else
					dev_agent_and_com[cur_dev_id_].second->start();
			}
			//初始化最后保存时间
			time_t curTm = time(0);
			map<string,time_t>::iterator iter = tmLastSaveTime.begin();
			for(;iter!=tmLastSaveTime.end();++iter)
				(*iter).second = curTm;
			return;
		}
		else
		{
			close_i();
			start_connect_timer();
		}
	}

	void device_session::handle_read_head(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if(!is_connected())
			return;
        if (!error)// || error.value() == ERROR_MORE_DATA
		{
			int nResult = receive_msg_ptr_->check_normal_msg_header(dev_agent_and_com[cur_dev_id_].second,bytes_transferred,CMD_QUERY,cur_msg_q_id_);
			if(nResult>0)//检查消息头
				start_read_body(nResult);
			else{
				close_all();
				start_connect_timer(moxa_config_ptr->connect_timer_interval);
			}
		}
		else{
			close_all();
			start_connect_timer(moxa_config_ptr->connect_timer_interval);
		}
	}

	void device_session::start_read_head(int msgLen)
	{
		if(msgLen>receive_msg_ptr_->space())
		{
			cout<<"data overflow !!!!"<<endl;
			return;
		}

		if(is_tcp())
		{
			boost::asio::async_read(socket(), boost::asio::buffer(receive_msg_ptr_->w_ptr(),
				msgLen),
#ifdef USE_STRAND
				strand_.wrap(
#endif
				boost::bind(&device_session::handle_read_head,this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
				)
#endif
				);
		}
		else
		{
			udp::endpoint sender_endpoint;
			usocket().async_receive_from(boost::asio::buffer(receive_msg_ptr_->w_ptr(),receive_msg_ptr_->space()),sender_endpoint,//msgLen
#ifdef USE_STRAND
				strand_.wrap(
#endif
				boost::bind(&device_session::handle_udp_read,this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
				)
#endif
				);
		}
	}

	void device_session::handle_udp_read(const boost::system::error_code& error,size_t bytes_transferred)
	{
		if(!is_connected())
			return;
		if (!error || error == boost::asio::error::message_size)
		{
 			int nResult = receive_msg_ptr_->check_normal_msg_header(dev_agent_and_com[cur_dev_id_].second,bytes_transferred,CMD_QUERY,cur_msg_q_id_);
			
			if(nResult == 0)
			{
				DevMonitorDataPtr curData_ptr(new Data);
				int nResult = receive_msg_ptr_->decode_msg_body(dev_agent_and_com[cur_dev_id_].second,curData_ptr,bytes_transferred);
				if(nResult==0)//查询数据解析正确
				{
					command_timeout_count_ = 0;
					if(boost::detail::thread::singleton<boost::threadpool::pool>::instance()
						.schedule(boost::bind(&device_session::handler_data,this,cur_dev_id_,curData_ptr)))
					{
						task_count_increase();
					}
				}
			}
			start_read_head(bytes_transferred);
			
			
			cout<< "接收长度---"<<bytes_transferred << std::endl;

		}
		else{
			cout<< error.message() << std::endl;
			close_all();
			start_connect_timer(moxa_config_ptr->connect_timer_interval);
		}
	}



	void device_session::start_read_body(int msgLen)
	{

		if(is_tcp())
		{
			boost::asio::async_read(socket(), boost::asio::buffer(receive_msg_ptr_->w_ptr(),
				msgLen),
#ifdef USE_STRAND
				strand_.wrap(
#endif
				boost::bind(&device_session::handle_read_body,this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
				)
#endif
				);
		}
		else
		{
			udp::endpoint sender_endpoint;
			usocket().async_receive_from(boost::asio::buffer(receive_msg_ptr_->w_ptr(),
				msgLen),
				sender_endpoint,
#ifdef USE_STRAND
				strand_.wrap(
#endif
				boost::bind(&device_session::handle_read_body,this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
				)
#endif
				);
		}

	}


	void device_session::handle_read_body(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if(!is_connected())
			return;
		if (!error)
		{

			//pTsmtAgentMsgPtr curData_ptr(new TsmtAgentMsg);
			//int nResult = receive_msg_ptr_->decode_msg_body(dev_agent_,curData_ptr,bytes_transferred);
			DevMonitorDataPtr curData_ptr(new Data);
			int nResult = receive_msg_ptr_->decode_msg_body(dev_agent_and_com[cur_dev_id_].second,curData_ptr,bytes_transferred);
			if(nResult==0)//查询数据解析正确
			{
				command_timeout_count_ = 0;
				if(boost::detail::thread::singleton<boost::threadpool::pool>::instance()
					.schedule(boost::bind(&device_session::handler_data,this,cur_dev_id_,curData_ptr)))
				{
					task_count_increase();
				}

				if(cur_msg_q_id_<dev_agent_and_com[cur_dev_id_].first->queryComm.size()-1)
					++cur_msg_q_id_;
				else
					cur_msg_q_id_=0;
				start_read_head(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].ackLen);
			}
			else if(nResult>0)//跳过数据(针对数据管理器等设备的非查询指令回复)
				start_read_head(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].ackLen);
			else{
				close_all();
				start_connect_timer(moxa_config_ptr->connect_timer_interval);
			}
		}
		else{
			close_all();
			start_connect_timer(moxa_config_ptr->connect_timer_interval);
		}
	}


	void device_session::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
	{
        /*if(!is_connected())
			return;
		if(!error)
		{
			//amend by lk 2013-11-26
			int nResult = receive_msg_ptr_->check_msg_header(dev_agent_and_com[cur_dev_id_].second->DevAgent(),
				                                             bytes_transferred,CMD_QUERY,cur_msg_q_id_);
			if(nResult == 0)
			{
				//cout<<"nResult:"<<nResult<<"transferred:"<<bytes_transferred<<endl;
				command_timeout_count_ = 0;
				if(cur_msg_q_id_<dev_agent_and_com[cur_dev_id_].first->queryComm.size()-1)
				{
					++cur_msg_q_id_;
					boost::this_thread::sleep(boost::posix_time::milliseconds(200));
					start_write(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandId,
						        dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].commandLen);
				}
				else
				{
					cur_msg_q_id_=0;
					DevMonitorDataPtr curData_ptr(new Data);
					if(receive_msg_ptr_->decode_msg(dev_agent_and_com[cur_dev_id_].second->DevAgent(),curData_ptr))
					{

						if(boost::detail::thread::singleton<boost::threadpool::pool>::instance()
							.schedule(boost::bind(&device_session::handler_data,this,cur_dev_id_,curData_ptr)))
						{
							task_count_increase();
						}
						cur_dev_id_ = next_dev_id();//切换到下一个设备
					}
					else{
						close_all();
						start_connect_timer();
						return;
					}
				}
				start_read(dev_agent_and_com[cur_dev_id_].first->queryComm[cur_msg_q_id_].ackLen);
			}
			else if(nResult>0){
				cout<<"nResult:"<<nResult<<"transferred:"<<bytes_transferred<<endl;
				start_read(nResult);
			}
			else if(nResult == -1){
				close_all();
				start_connect_timer();
				return;
			}
		}
		else{
			close_all();
			start_connect_timer();
			return;
        }*/
	}
	void device_session::handle_write(const boost::system::error_code& error,size_t bytes_transferred)
	{
		if(!is_connected())
			return;
		if(error){
			close_all();
			start_connect_timer();
		}
	}


	bool device_session::ItemValueIsAlarm(string devId,float fValue,DevParamerMonitorItem &ItemInfo
		                                  ,dev_alarm_state &alarm_state)
	{
		if(ItemInfo.nMonitoringType == 0)//模拟量
		{
			if(fValue > ItemInfo.dUpperLimit)
			{
				if(mapItemAlarmRecord[devId].find(ItemInfo.nMonitoringIndex)!=mapItemAlarmRecord[devId].end())
				{
					if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first == 0){

						if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second == 
							run_config_ptr[devId]->alarm_detect_max_count){//是否大于最大检测次数
							alarm_state = upper_alarm;
							++mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second;
							return true;
						}
						else	{
								if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second>=run_config_ptr[devId]->alarm_detect_max_count+1)
								mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second=run_config_ptr[devId]->alarm_detect_max_count+1;
							else
								++mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second;
						}
					}
					else{//第一次数据超上限
						mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first = 0;
						mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second = 1;

						//同时记录首次告警时间以及告警状态----2015-7-28
						time_t curTime = time(0);
						tm *ltime = localtime(&curTime);
						mapItemAlarmStartTime[devId][ItemInfo.nMonitoringIndex]=std::pair<int,tm>(upper_alarm,*ltime);
					}
				}
				else
				{
					//无报警则添加该报警项
					mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first=0;
					mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second=1;
					//同时记录首次告警时间以及告警状态----2015-7-28
					time_t curTime = time(0);
					tm *ltime = localtime(&curTime);
					mapItemAlarmStartTime[devId][ItemInfo.nMonitoringIndex]=std::pair<int,tm>(upper_alarm,*ltime);
				}
			}
			else if(fValue < ItemInfo.dLowerLimit)
			{
				if(mapItemAlarmRecord[devId].find(ItemInfo.nMonitoringIndex)!=mapItemAlarmRecord[devId].end())
				{
					if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first == 1){
						if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second == 
							run_config_ptr[devId]->alarm_detect_max_count){
							alarm_state = lower_alarm;
							++mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second;
							return true;
						}
						else
						{
							
							if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second>=
								run_config_ptr[devId]->alarm_detect_max_count+1)
								mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second=run_config_ptr[devId]->alarm_detect_max_count+1;
							else
								++mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second;
						}
					}
					else{//第一次数据低于下限
						mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first = 1;
						mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second = 1;
						//同时记录首次告警时间以及告警状态----2015-7-28
						time_t curTime = time(0);
						tm *ltime = localtime(&curTime);
						mapItemAlarmStartTime[devId][ItemInfo.nMonitoringIndex]=std::pair<int,tm>(lower_alarm,*ltime);
					}
				}
				else{//无报警则添加该报警项
					mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first=1;
					mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second=1;
					//同时记录首次告警时间以及告警状态----2015-7-28
					time_t curTime = time(0);
					tm *ltime = localtime(&curTime);
					mapItemAlarmStartTime[devId][ItemInfo.nMonitoringIndex]=std::pair<int,tm>(lower_alarm,*ltime);
				}
			}else{//查找此报警是否已经存在
				if(mapItemAlarmRecord[devId].find(ItemInfo.nMonitoringIndex)!=mapItemAlarmRecord[devId].end()){
					
					
					if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second>=run_config_ptr[devId]->alarm_detect_max_count)
						alarm_state = resume_normal;
					mapItemAlarmRecord[devId].erase(ItemInfo.nMonitoringIndex);
				}
			}

		}
		else //状态量
		{
			//报警
			if(fValue==ItemInfo.dLowerLimit)//以下限指示来确定报警值
			{
				//判断当前item是否在报警
				if(mapItemAlarmRecord[devId].find(ItemInfo.nMonitoringIndex)
					!=mapItemAlarmRecord[devId].end()){
					//是否达到检测次数(2次)
					if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second==
						run_config_ptr[devId]->alarm_detect_max_count){
						alarm_state = state_alarm;
						++mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second;
						cout<<"devId:"<<devId<<"-second->"<<mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second<<endl;
						return true;
					}
					else{
						if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second>=
							run_config_ptr[devId]->alarm_detect_max_count+1)
							mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second=run_config_ptr[devId]->alarm_detect_max_count+1;
						else
							++mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second;
					}
				}
				else{//若当前是越下限或低功率报警,则修改后越上限报警
					mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].first=0;
					mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second=1;
					//同时记录首次告警时间以及告警状态----2015-7-28
					time_t curTime = time(0);
					tm *ltime = localtime(&curTime);
					mapItemAlarmStartTime[devId][ItemInfo.nMonitoringIndex]=std::pair<int,tm>(state_alarm,*ltime);
				}
			}
			else{
				//查找此报警是否已经存在
				if(mapItemAlarmRecord[devId].find(ItemInfo.nMonitoringIndex)
					!=mapItemAlarmRecord[devId].end()){

				  if(mapItemAlarmRecord[devId][ItemInfo.nMonitoringIndex].second>=run_config_ptr[devId]->alarm_detect_max_count)
						alarm_state = resume_normal;
				  else
				  {
					  //移除该报警项
					  mapItemAlarmStartTime[devId].erase(ItemInfo.nMonitoringIndex);
				  }
					mapItemAlarmRecord[devId].erase(ItemInfo.nMonitoringIndex);
					//alarm_state = resume_normal;
				}
			}
		}
		return false;
	}

	//清除所有报警状态
	void device_session::clear_all_alarm()
	{
		boost::recursive_mutex::scoped_lock lock(alarm_state_mutex);
		mapItemAlarmStartTime.clear();//报警项开始时间清除
		mapItemAlarmRecord.clear();   //报警记录清除
	}

	void device_session::check_alarm_state(string sDevId,DevMonitorDataPtr curDataPtr,bool bMonitor)
	{
		//非检测时间段不进行报警检测...

		DevParamerInfo info_;
		map<string,DevParamerInfo>::iterator iter = modleInfos.mapDevInfo.find(sDevId);
		if(iter== modleInfos.mapDevInfo.end())
			return;

		boost::recursive_mutex::scoped_lock lock(alarm_state_mutex);
		map<int,DevParamerMonitorItem>::iterator iterItem = iter->second.mapMonitorItem.begin();
		for(;iterItem!=iter->second.mapMonitorItem.end();++iterItem)
		{
			if(!(*iterItem).second.bAlarmEnable)
				continue;
			//数据没有更新或没有抵达则不进行告警判断
			if(curDataPtr->datainfoBuf[(*iterItem).first].bUpdate==false)
				continue;

			dev_alarm_state curState = invalid_alarm;
			double dbValue =curDataPtr->datainfoBuf[(*iterItem).first].fValue;
			if(ItemValueIsAlarm(sDevId,dbValue,(*iterItem).second,curState))
			{
				//有报警产生，存数据库，并通知客户端
				//记录数据库
				//time_t curTime = time(0);
				//tm *ltime = localtime(&curTime);
				tm ltime = mapItemAlarmStartTime[sDevId][(*iterItem).first].second;
                GetInst(DbManager).AddOthDevAlarm(&ltime,iter->second.sStationNumber,
					                                    iter->second.sDevNum,(*iterItem).first,curState);
				//mapItemAlarmStartTime[sDevId][(*iterItem).first] = std::pair<int,tm>(curState,*ltime);
				
				if(bMonitor==true)//如果在监测时间段则通知客户端
				{
					//分发设备监控量报警到客户端
					std::string alramTm = str(boost::format("%1%/%2%/%3% %4%:%5%:%6%")
																	%(ltime.tm_year+1900)
																	%(ltime.tm_mon+1)
																	%ltime.tm_mday
																	%ltime.tm_hour
																	%ltime.tm_min
																	%ltime.tm_sec);
					send_alarm_state_message(modleInfos.sStationNumber,sDevId
										,(*iter).second.sDevName,(*iterItem).second.nMonitoringIndex
										,(*iterItem).second.sMonitoringName
										,(e_DevType)modleInfos.mapDevInfo[sDevId].nDevType
										,curState,alramTm,mapItemAlarmStartTime[sDevId].size());
					//根据报警等级确定是否进行短信通知用户
					string sAlarmContent = str(boost::format("%1%%2%%3%[时间:%4%]")
												%(*iter).second.sDevName
												%(*iterItem).second.sMonitoringName
												%CONST_STR_ALARM_CONTENT[(int)curState]
												%alramTm
												);
					sendSmsAndCallPhone((*iterItem).second.nAlarmLevel,sAlarmContent);
				}
			}
			else
			{
				if(curState == resume_normal)
				{
					time_t curTime = time(0);
					tm *ltime = localtime(&curTime);//恢复时间
					//报警恢复，存数据库，并通知客户端
                    GetInst(DbManager).UpdateOthDevAlarm(ltime,mapItemAlarmStartTime[sDevId][(*iterItem).first].second,
						                                       iter->second.sStationNumber,iter->second.sDevNum,(*iterItem).first);
					//移除该报警项
					mapItemAlarmStartTime[sDevId].erase((*iterItem).first);
					//广播设备监控量报警到客户端
					std::string alramTm = str(boost::format("%1%/%2%/%3% %4%:%5%:%6%")
													%(ltime->tm_year+1900)
													%(ltime->tm_mon+1)
													%ltime->tm_mday
													%ltime->tm_hour
													%ltime->tm_min
													%ltime->tm_sec);
					send_alarm_state_message(modleInfos.sStationNumber,sDevId
											,(*iter).second.sDevName,(*iterItem).second.nMonitoringIndex
											,(*iterItem).second.sMonitoringName
											,(e_DevType)modleInfos.mapDevInfo[sDevId].nDevType
											,curState,alramTm,mapItemAlarmStartTime[sDevId].size());

					string sAlarmContent = str(boost::format("%1%%2%%3%[时间:%4%]")
												%(*iter).second.sDevName
												%(*iterItem).second.sMonitoringName
												%CONST_STR_ALARM_CONTENT[(int)curState]
												%alramTm
												);
					sendSmsAndCallPhone((*iterItem).second.nAlarmLevel,sAlarmContent);

				}
			}
		}
	}
	
	void device_session::sendSmsToUsers(int nLevel,string &sContent)
	{
		vector<SendMSInfo>& smsInfo=GetInst(StationConfig).get_sms_user_info();
		for(int i=0;i<smsInfo.size();++i)
		{
			if(smsInfo[i].iAlarmLevel==nLevel)
			{
				string sCenterId = GetInst(LocalConfig).sms_center_number();
				//扩平台需要，暂时删去
				//GetInst(CSmsTraffic).SendSMSContent(sCenterId,smsInfo[i].sPhoneNumber,sContent);
			}
		}

	}
	//用来执行通用指令,根据具体参数,组织命令字符串,同时发送命令消息
	bool device_session::excute_general_command(int cmdType,devCommdMsgPtr lpParam,e_ErrorCode &opResult)
	{
		CommandUnit adjustTmCmd;
		dev_agent_and_com[lpParam->sdevid()].second->GetSignalCommand(lpParam,adjustTmCmd);
		if(adjustTmCmd.commandLen>0)
		{
			start_write(adjustTmCmd.commandId ,adjustTmCmd.commandLen);
			opResult=EC_CMD_SEND_SUCCEED;
			return true;
		}
		return false;
	}

	//执行通用指令
	bool device_session::excute_command(int cmdType,devCommdMsgPtr lpParam,e_ErrorCode &opResult)
	{
		if(get_con_state()!=con_connected)
			return false;
		switch(cmdType)
		{
		case MSG_0401_SWITCH_OPR:
			{
				if(lpParam->cparams_size()!=1)
					return false;
				int nChannel = atoi(lpParam->cparams(0).sparamvalue().c_str());
				if(dev_agent_and_com.find(lpParam->sdevid())!=dev_agent_and_com.end()){
					if(dev_agent_and_com[lpParam->sdevid()].first->switchComm.size()>nChannel){
						start_write(dev_agent_and_com[lpParam->sdevid()].first->switchComm[nChannel].commandId ,
									dev_agent_and_com[lpParam->sdevid()].first->switchComm[nChannel].commandLen );
					}
				}
			}
			break;
		case MSG_CONTROL_MOD_SWITCH_OPR:  
			{
				if(lpParam->cparams_size()!=1)
					return false;
				int nMode = 0;//atoi(lpParam->cparams(0).sparamvalue().c_str());
				if(dev_agent_and_com.find(lpParam->sdevid())!=dev_agent_and_com.end()){
					if(dev_agent_and_com[lpParam->sdevid()].first->switch2Comm.size()>nMode){
						start_write(dev_agent_and_com[lpParam->sdevid()].first->switch2Comm[nMode].commandId ,
							        dev_agent_and_com[lpParam->sdevid()].first->switch2Comm[nMode].commandLen );
					}
				}
			}
			break;
		case MSG_ADJUST_TIME_SET_OPR:
			{
				if(lpParam->cparams_size()!=0) 
					return false;
				if(dev_agent_and_com.find(lpParam->sdevid())!=dev_agent_and_com.end()){

					CommandUnit adjustTmCmd;
					vector<string> params;
					////amend by lk 2013-11-26
                    //dev_agent_and_com[lpParam->sdevid()].second->DevAgent()->HxGetSignalCmd(CMD_ADJUST_TIME,params,adjustTmCmd);
                    //if(adjustTmCmd.commandLen>0)
                    //	start_write(adjustTmCmd.commandId ,adjustTmCmd.commandLen);
				}
			}
			break;
		case MSG_GENERAL_COMMAND_OPR:
			{
				excute_general_command(cmdType,lpParam,opResult);
			}
		}

		return true;
	}
}
