#include "session.h"
#include "../StationConfig.h"
#include "SvcMgr.h"
#include "LocalConfig.h"
//#include "../sms/SmsTraffic.h"
//#include <glog/logging.h>
/*************************************************************************
* 说明：这是一个基类，客户端与设备连接handler需从此类派生，重载相关虚接口
        完成消息的插队，验证，通知派发操作。
*
*************************************************************************/
namespace net
{

	session::session(boost::asio::io_service& io_service)
		:socket_(io_service)
		,udp_socket_(io_service)
		,bTcp_(true)
	{

	}

	session::~session(void)
	{

	}

	tcp::socket& session::socket()
	{
		boost::mutex::scoped_lock lock(socket_mutex_);
		//socket_.set_option(tcp::no_delay(true));
		return socket_;
	}

	udp::socket& session::usocket()
	{
		boost::mutex::scoped_lock(socket_mutex_);
		return udp_socket_;
	}
	
	bool  session::is_tcp()
	{
		return bTcp_;
	}

	void session::set_tcp(bool bTcp)
	{
		bTcp_ = bTcp;
	}
	tcp::endpoint session::get_addr()
	{
		return socket().remote_endpoint();
	}

	udp::endpoint session::get_udp_addr()
	{
		return usocket().remote_endpoint();
	}

	void session::close_i()
	{
		boost::system::error_code ignored_ec;
		boost::mutex::scoped_lock lock(socket_mutex_);
		if(bTcp_)
		{
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			socket_.close(ignored_ec);
		}else
		{
			udp_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			udp_socket_.close(ignored_ec);
		}

	}

	void session::handle_read_body(const boost::system::error_code& error,
		size_t bytes_transferred)
	{
	}

	void session::handle_read_head(const boost::system::error_code& error,
		size_t bytes_transferred)
	{
		
	}

	bool session::sendMessage(e_MsgType _type,googleMsgPtr gMsgPtr)//google::protobuf::Message *
	{
		return false;
	}


	void session::handle_write(const boost::system::error_code& error,size_t bytes_transferred)
	{
	}
	
	//打包发送实时多媒体数据（汇鑫760音频）
	void session::send_monitor_data_message(string sStationid,string sDevid,e_DevType devType,
											unchar_ptr curData,DevParamerMonitorItem &mapMonitorItem)
	{
		devDataNfyMsgPtr dev_cur_data_ptr(new DevDataNotify);
		dev_cur_data_ptr->set_edevtype(devType);
		dev_cur_data_ptr->set_sstationid(sStationid);
		dev_cur_data_ptr->set_sdevid(sDevid);

		devDataNfyMsgPtr dev_cur_data_tosvr_ptr;
		if(GetInst(LocalConfig).upload_use()==true)
		{
			dev_cur_data_tosvr_ptr = devDataNfyMsgPtr(new DevDataNotify);
			dev_cur_data_tosvr_ptr->set_edevtype(devType);
			dev_cur_data_tosvr_ptr->set_sstationid(sStationid);
			dev_cur_data_tosvr_ptr->set_sdevid(sDevid);
		}

		DevDataNotify_eCellMsg *cell = dev_cur_data_ptr->add_ccelldata();
		cell->set_ecelltype((e_CellType)mapMonitorItem.nMonitoringType);
		cell->set_scellid(boost::lexical_cast<string>(mapMonitorItem.nMonitoringIndex-59));
		cell->set_scellname(QString::fromLocal8Bit(mapMonitorItem.sMonitoringName.c_str()).toUtf8().data());

		cell->set_baudiovalue(&curData->at(0),curData->size());//
		if(dev_cur_data_tosvr_ptr!=0)
		{
			if(mapMonitorItem.bIsUpload==true)
			{
				DevDataNotify_eCellMsg *upcell = dev_cur_data_tosvr_ptr->add_ccelldata();
				upcell->set_ecelltype((e_CellType)mapMonitorItem.nMonitoringType);
				upcell->set_scellid(boost::lexical_cast<string>(mapMonitorItem.nMonitoringIndex-59));
				upcell->set_scellname(QString::fromLocal8Bit(mapMonitorItem.sMonitoringName.c_str()).toUtf8().data());
				//cell->set_scellvalue(sValue);
				upcell->set_baudiovalue(&curData->at(0),curData->size());
			}
		}
		GetInst(SvcMgr).send_monitor_data_to_client(sStationid,sDevid,dev_cur_data_ptr,dev_cur_data_tosvr_ptr);
	}
	//打包发送761数据（Mp3）
	void session::send_monitor_data_message_ex(string sStationid,string sDevid,e_DevType devType,
								unsigned char *curData,int nDataLen,DevParamerMonitorItem &mapMonitorItem)
	{
		devDataNfyMsgPtr dev_cur_data_ptr(new DevDataNotify);
		dev_cur_data_ptr->set_edevtype(devType);
		dev_cur_data_ptr->set_sstationid(sStationid);
		dev_cur_data_ptr->set_sdevid(sDevid);

		devDataNfyMsgPtr dev_cur_data_tosvr_ptr;
		if(GetInst(LocalConfig).upload_use()==true)
		{
			dev_cur_data_tosvr_ptr = devDataNfyMsgPtr(new DevDataNotify);
			dev_cur_data_tosvr_ptr->set_edevtype(devType);
			dev_cur_data_tosvr_ptr->set_sstationid(sStationid);
			dev_cur_data_tosvr_ptr->set_sdevid(sDevid);
		}

		DevDataNotify_eCellMsg *cell = dev_cur_data_ptr->add_ccelldata();
		cell->set_ecelltype((e_CellType)mapMonitorItem.nMonitoringType);
		cell->set_scellid(boost::lexical_cast<string>(mapMonitorItem.nMonitoringIndex-59));
		cell->set_scellname(QString::fromLocal8Bit(mapMonitorItem.sMonitoringName.c_str()).toUtf8().data());
		cell->set_baudiovalue(curData,nDataLen);
		if(dev_cur_data_tosvr_ptr!=0)
		{
			if(mapMonitorItem.bIsUpload==true)
			{
				DevDataNotify_eCellMsg *upcell = dev_cur_data_tosvr_ptr->add_ccelldata();
				upcell->set_ecelltype((e_CellType)mapMonitorItem.nMonitoringType);
				upcell->set_scellid(boost::lexical_cast<string>(mapMonitorItem.nMonitoringIndex-59));
				upcell->set_scellname(QString::fromLocal8Bit(mapMonitorItem.sMonitoringName.c_str()).toUtf8().data());
				upcell->set_baudiovalue(curData,nDataLen);
			}
		}
		GetInst(SvcMgr).send_monitor_data_to_client(sStationid,sDevid,dev_cur_data_ptr,dev_cur_data_tosvr_ptr);
	}

	//打包发送实时数据消息
	void session::send_monitor_data_message(string sStationid,string sDevid,e_DevType devType,
											DevMonitorDataPtr curData,map<int,DevParamerMonitorItem> &mapMonitorItem)
	{
		devDataNfyMsgPtr dev_cur_data_ptr(new DevDataNotify);
		dev_cur_data_ptr->set_edevtype(devType);
		dev_cur_data_ptr->set_sstationid(sStationid);
		dev_cur_data_ptr->set_sdevid(sDevid);

		devDataNfyMsgPtr dev_cur_data_tosvr_ptr;
		if(GetInst(LocalConfig).upload_use()==true)
		{
			dev_cur_data_tosvr_ptr = devDataNfyMsgPtr(new DevDataNotify);
			dev_cur_data_tosvr_ptr->set_edevtype(devType);
			dev_cur_data_tosvr_ptr->set_sstationid(sStationid);
			dev_cur_data_tosvr_ptr->set_sdevid(sDevid);
		}

		map<int,DevParamerMonitorItem>::iterator cell_iter = mapMonitorItem.begin();
		for(;cell_iter!=mapMonitorItem.end();++cell_iter)
		{
			int cellId = (*cell_iter).first;
			
			//未更新的监测量
			if(curData->datainfoBuf[cellId].bUpdate==false)
				continue;

			DevDataNotify_eCellMsg *cell = dev_cur_data_ptr->add_ccelldata();
			cell->set_ecelltype((e_CellType)(*cell_iter).second.nMonitoringType);
			cell->set_scellid(boost::lexical_cast<string>((*cell_iter).second.nMonitoringIndex));
			cell->set_scellname(QString::fromLocal8Bit((*cell_iter).second.sMonitoringName.c_str()).toUtf8().data());
			
			string  sValue = str(boost::format("%.2f")%curData->datainfoBuf[cellId].fValue);
			cell->set_scellvalue(sValue);

			if(dev_cur_data_tosvr_ptr!=0)
			{
				if((*cell_iter).second.bIsUpload==true)
				{
					DevDataNotify_eCellMsg *upcell = dev_cur_data_tosvr_ptr->add_ccelldata();
					upcell->set_ecelltype((e_CellType)(*cell_iter).second.nMonitoringType);
					upcell->set_scellid(boost::lexical_cast<string>((*cell_iter).second.nMonitoringIndex));
					upcell->set_scellname(QString::fromLocal8Bit((*cell_iter).second.sMonitoringName.c_str()).toUtf8().data());
					string  sValue = str(boost::format("%.2f")%curData->datainfoBuf[(*cell_iter).first].fValue);
					upcell->set_scellvalue(sValue);
				}
			}

		}

		GetInst(SvcMgr).send_monitor_data_to_client(sStationid,sDevid,dev_cur_data_ptr,dev_cur_data_tosvr_ptr);
	}

	//打包发送设备连接状态消息
	void session::send_net_state_message(string sStationid,string sDevid,string sDevName,e_DevType devType,
		                                 con_state netState)
	{
		devNetNfyMsgPtr  dev_net_nfy_ptr(new DevNetStatusNotify);
		DevNetStatus *dev_n_s = dev_net_nfy_ptr->add_cdevcurnetstatus();
		dev_n_s->set_edevtype(devType);
		dev_n_s->set_sstationid(sStationid);
		dev_n_s->set_sdevid(sDevid);
		dev_n_s->set_sdevname(QString::fromLocal8Bit(sDevName.c_str()).toUtf8().data());
		dev_n_s->set_enetstatus((DevNetStatus_e_NetStatus)netState);

		GetInst(SvcMgr).send_dev_net_state_to_client(sStationid,sDevid,dev_net_nfy_ptr);
	}

	void session::send_work_state_message( string sStationid,string sDevid,string sDevName,e_DevType devType, dev_run_state runState )
	{
		devWorkNfyMsgPtr dev_run_nfy_ptr(new DevWorkStatusNotify);// dev_run_nfy;
		DevWorkStatus *dev_n_s = dev_run_nfy_ptr->add_cdevcurworkstatus();
		dev_n_s->set_edevtype(devType);
		dev_n_s->set_sstationid(sStationid);
		dev_n_s->set_sdevid(sDevid);
		dev_n_s->set_sdevname(QString::fromLocal8Bit(sDevName.c_str()).toUtf8().data());
		dev_n_s->set_eworkstatus((DevWorkStatus_e_WorkStatus)runState);

		GetInst(SvcMgr).send_dev_work_state_to_client(sStationid,sDevid,dev_run_nfy_ptr);
	}
	
	void session::send_alarm_state_message(string sStationid,string sDevid,string sDevName,
										   int nCellId,string sCellName,e_DevType devType,dev_alarm_state alarmState,
		                                   string sStartTime,int alarmCount)
	{
		devAlarmNfyMsgPtr dev_alarm_nfy_ptr(new DevAlarmStatusNotify);
		DevAlarmStatus *dev_n_s = dev_alarm_nfy_ptr->add_cdevcuralarmstatus();
		dev_n_s->set_edevtype(devType);
		dev_n_s->set_sstationid(sStationid);
		dev_n_s->set_sdevid(sDevid);
		dev_n_s->set_sdevname(QString::fromLocal8Bit(sDevName.c_str()).toUtf8().data());
		dev_n_s->set_nalarmcount(alarmCount);
		DevAlarmStatus_eCellAlarmMsg *dev_cell_alarm = dev_n_s->add_ccellalarm();
		std::string scellid = str(boost::format("%1%")%nCellId);
		dev_cell_alarm->set_scellid(scellid);
		dev_cell_alarm->set_scellname(QString::fromLocal8Bit(sCellName.c_str()).toUtf8().data());
		dev_cell_alarm->set_sstarttime(sStartTime);
		dev_cell_alarm->set_ccellstatus((e_AlarmStatus)alarmState);

		GetInst(SvcMgr).send_dev_alarm_state_to_client(sStationid,sDevid,dev_alarm_nfy_ptr);
	}

	void session::send_command_execute_result_message(string sStationid,string sDevid,e_DevType devType,string sDevName,
													  string sUsrName,e_MsgType nMsgType,e_ErrorCode eResult)
	{
		devCommdRsltPtr ackMsgPtr(new DeviceCommandResultNotify);
		ackMsgPtr->set_sstationid(sStationid);
		ackMsgPtr->set_sdevid(sDevid);
		ackMsgPtr->set_sdevname(QString::fromLocal8Bit(sDevName.c_str()).toUtf8().data());
		ackMsgPtr->set_edevtype(devType);
		ackMsgPtr->set_eerrorid(eResult);
		ackMsgPtr->set_soperuser(QString::fromLocal8Bit(sUsrName.c_str()).toUtf8().data());

		GetInst(SvcMgr).send_command_execute_result(sStationid,sDevid,nMsgType,ackMsgPtr);
	}
 
	//判断是否发短信与打电话
	void session::sendSmsAndCallPhone(int nAlarmLevel,string sContent)
	{

		vector<SendMSInfo>& smsInfo=GetInst(StationConfig).get_sms_user_info();
		
		for(int i=0;i<smsInfo.size();++i)
		{
			if(smsInfo[i].iAlarmLevel==nAlarmLevel)
			{
				//如果告警级别需要发送短信，且短信使能打开
				if((nAlarmLevel==SMS || nAlarmLevel==SMSANDTEL)&&
					GetInst(LocalConfig).sms_use())
				{
					string sCenterId = GetInst(LocalConfig).sms_center_number();
				}
                //发送短信2016-3-22
                //GetInst(DbManager).WriteCallTask(smsInfo[i].sPhoneNumber,sContent,nAlarmLevel);
			}
		}
	}
}
