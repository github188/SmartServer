#include "Link_message.h"
#include"../../../utility.h"
#include "./snmp_pp/snmp_pp.h"

#define NUM_SYS_VBS	6
//信号电平
#define rflevel	"1.3.6.1.4.1.8201.5.7.1.1.4.0"
//锁定状态
#define signalLock	"1.3.6.1.4.1.8201.5.7.1.1.1.0"
//误码率
#define singalber	"1.3.6.1.4.1.8201.5.7.1.1.2.0"
//载噪比
#define signalcn	"1.3.6.1.4.1.8201.5.7.1.1.3.0"
//统计码率
#define totalrate    "1.3.6.1.4.1.8201.5.7.1.1.5.0"
//频率
#define frequency	"1.3.6.1.4.1.8201.5.7.1.1.7.0"

namespace hx_net
{

    Link_message::Link_message(session_ptr pSession,DeviceInfo &devInfo)
        :m_pSession(pSession)
        ,d_devInfo(devInfo)
        ,d_task_queue_ptr(new TaskQueue< DevMonitorDataPtr >)
    {
    }

    Link_message::~Link_message(void)
    {
    }
    int Link_message::check_msg_header(unsigned char *data,int nDataLen,CmdType cmdType,int number)
    {
        return RE_UNKNOWDEV;
    }

     int Link_message::decode_msg_body(Snmp *snmp,DevMonitorDataPtr data_ptr,CTarget *target)
     {
         switch(d_devInfo.nDevProtocol)
         {
         case LINK_DEVICE:
             {
                 switch (d_devInfo.nSubProtocol)
                 {
                 case LINK_STATELITE_RECEVIE:
                     return parse_SatelliteReceive_data(snmp,data_ptr,target);
                 default:
                     return RE_NOPROTOCOL;
                 }
             }
             break;
         }
         return RE_UNKNOWDEV;
     }

    int Link_message::decode_msg_body(unsigned char *data,DevMonitorDataPtr data_ptr,int nDataLen)
    {
        return RE_UNKNOWDEV;
    }

    bool Link_message::IsStandardCommand()
    {
        return false;
    }

    void Link_message::GetSignalCommand(devCommdMsgPtr lpParam,CommandUnit &cmdUnit)
    {
    }

    void Link_message::GetAllCmd( CommandAttribute &cmdAll )
    {
    }


    void aysnc_callback(int reason, Snmp *session,
                Pdu &pdu, SnmpTarget &target, void *cd)
    {
        if (cd)
          ((Link_message*)cd)->Link_Callback(reason, session, pdu, target);
    }


    void Link_message::Link_Callback(int reason, Snmp *session,Pdu &pdu, SnmpTarget &target)
    {
        int pdu_error = pdu.get_error_status();
        if (pdu_error)
            return;
        if (pdu.get_vb_count() == 0)
            return;

        switch (d_devInfo.nSubProtocol)
        {
        case LINK_STATELITE_RECEVIE:
            return parse_Satellite_data_(pdu,target);
        }
         cout<<"satellite_Callback  reason="<<d_devInfo.sDevNum<<endl;
    }

    void Link_message::parse_Satellite_data_(Pdu &pdu, SnmpTarget &target)
    {
        DevMonitorDataPtr cur_data_ptr  = d_task_queue_ptr->GetTask();
        if(cur_data_ptr==NULL)
            return;
        //boost::recursive_mutex::scoped_lock lock(data_mutex);
        for (int i=0; i<pdu.get_vb_count(); i++)
        {
            Vb nextVb;
            pdu.get_vb(nextVb, i);
            string cur_oid = nextVb.get_printable_oid();//Oid
            string cur_value =nextVb.get_printable_value();
            //if(cur_value.empty())
             //   continue;
            DataInfo dainfo;
            if(cur_oid == rflevel){
                if(cur_value.empty())
                    cur_value ="-94";
                dainfo.sValue = cur_value;
                dainfo.bType=false;
                dainfo.fValue =  atof(cur_value.c_str());
                cur_data_ptr->mValues[0] = dainfo;
            }else if(cur_oid == signalLock){
                dainfo.bType=true;
                int nRlt = cur_value.find("CB F8 B6 A8");
                if(nRlt!=-1){//信号锁
                    dainfo.sValue = "1";
                    dainfo.fValue=1.0f;
                }else{
                    dainfo.sValue = "0";
                    dainfo.fValue=0.0f;
                }
                cur_data_ptr->mValues[1] = dainfo;
            }else if(cur_oid == singalber){
                if(cur_value.empty())
                    cur_value ="45";
                dainfo.bType=false;
                dainfo.sValue = cur_value;
                string_replace(cur_value,"dB","");
                string_replace(cur_value," ","");
                if(cur_value=="N/A")
                    dainfo.fValue=0.0f;
                else
                    dainfo.fValue =  atof(cur_value.c_str());
                cur_data_ptr->mValues[2] = dainfo;
            }else if(cur_oid == signalcn){
                if(cur_value.empty())
                    cur_value ="30";
                dainfo.bType=false;
                dainfo.sValue = cur_value;
                string_replace(cur_value,"dB","");
                string_replace(cur_value," ","");
                dainfo.fValue = atof(cur_value.c_str());
                cur_data_ptr->mValues[3] = dainfo;
            }else if(cur_oid == totalrate){
                if(cur_value.empty())
                    cur_value ="10";
                dainfo.bType=false;
                dainfo.sValue = cur_value;
                dainfo.fValue = atof(cur_value.c_str());
                cur_data_ptr->mValues[4] = dainfo;
            }else if(cur_oid == frequency){
                if(cur_value.empty())
                    cur_value ="658";
                dainfo.bType=false;
                dainfo.sValue = cur_value;
                string_replace(cur_value,"MHz","");
                string_replace(cur_value," ","");
                dainfo.fValue = atof(cur_value.c_str());
                cur_data_ptr->mValues[5] = dainfo;
            }
        }
        if(cur_data_ptr->mValues.size()>0){
            //DevMonitorDataPtr  curCloneData_ptr(new Data(*d_data_ptr.get()));
            m_pSession->start_handler_data(d_devInfo.sDevNum,cur_data_ptr);
        }

    }

    int  Link_message::parse_SatelliteReceive_data(Snmp *snmp,DevMonitorDataPtr data_ptr,CTarget *target)
    {
        if(d_task_queue_ptr->get_Task_Size()>10)
            return -1;
        //cout<<"snmp--------task-----size -------"<<d_task_queue_ptr->get_Task_Size()<<endl;
        d_task_queue_ptr->SubmitTask(data_ptr);
        Pdu pdu;
        Vb vbl[NUM_SYS_VBS];
        vbl[0].set_oid(rflevel);
        vbl[1].set_oid(signalLock);
        vbl[2].set_oid(singalber);
        vbl[3].set_oid(signalcn);
        vbl[4].set_oid(totalrate);
        vbl[5].set_oid(frequency);
        for (int i=0; i<NUM_SYS_VBS;i++)
            pdu += vbl[i];
        //d_data_ptr = data_ptr;//保存
        int status = snmp->get(pdu,*target, aysnc_callback,this);
        if (status){
            return -1;
        }else{

           /*
             int vbcount = pdu.get_vb_count();
            int index = 0; //解析位置
            DataInfo dainfo;
            if ( vbcount == NUM_SYS_VBS ) {
                pdu.get_vblist(vbl, vbcount);
                 string s=vbl[0].get_printable_value();
                 dainfo.sValue = s;
                 dainfo.bType=false;
                 dainfo.fValue =  atof(s.c_str());
                 data_ptr->mValues[index++] = dainfo;

                dainfo.bType=true;
                s=vbl[1].get_printable_value();
                int nRlt = s.find("CB F8 B6 A8");
                if(nRlt!=-1){//信号锁
                    dainfo.sValue = "1";
                    dainfo.fValue=1.0f;
                }
                else{
                    dainfo.sValue = "0";
                    dainfo.fValue=0.0f;
                }
                data_ptr->mValues[index++] = dainfo;

                dainfo.bType=false;
                s=vbl[2].get_printable_value();
                dainfo.sValue = s;
                string_replace(s,"dB","");
                string_replace(s," ","");
                if(s=="N/A")
                    dainfo.fValue=0.0f;
                else{
                    dainfo.fValue =  atof(s.c_str());
                }
                data_ptr->mValues[index++] = dainfo;

                dainfo.bType=false;
                s=vbl[3].get_printable_value();
                dainfo.sValue = s;
                string_replace(s,"dB","");
                string_replace(s," ","");
                dainfo.fValue = atof(s.c_str());
                data_ptr->mValues[index++] = dainfo;

                s=vbl[4].get_printable_value();
                dainfo.sValue = s;
                dainfo.fValue = atof(s.c_str());
                data_ptr->mValues[index++] = dainfo;

                s=vbl[5].get_printable_value();
                dainfo.sValue = s;
                string_replace(s,"MHz","");
                string_replace(s," ","");
                dainfo.fValue = atof(s.c_str());
                data_ptr->mValues[index++] = dainfo;

                return 0;
            }*/
            return 0;
       }
        return -1;

    }

}
