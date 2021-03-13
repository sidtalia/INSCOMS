#ifndef _INSCOMS_H_
#define _INSCOMS_H_

#include"Arduino.h"
#include"INSPARAMS.h"
#include"INS_COMMON.h"
#if 0
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#endif

#define COM_BAUD 921600

class GCS
{
public:
	ins_data_struct ins_states;
	sensor_data_struct sensor_states;
	external_data_struct external_inputs;
	int16_t msg_len;
	uint8_t mode;
	long transmit_stamp, transmit_ins_state, transmit_sensor_data, received_stamp,failsafe_stamp;
	bool failsafe = false;
	long transmit_time;
	GCS()
	{
		Serial1.begin(COM_BAUD);
		received_stamp = transmit_stamp = failsafe_stamp = transmit_sensor_data = transmit_ins_state = millis();
		mode = 0x01;
	}
	
	void write_To_Port(int32_t a,int bytes)
	{
		uint8_t b[4];
		for(int i = 0;i<bytes;i++)
		{
			b[i] = a>>(8*(i)); //last 8 bits
			Serial1.write(b[i]);
		}
	}

	bool Get_Offsets(int16_t A[3], int16_t G[3], int16_t M[3], int16_t &T,int16_t gain[3])
	{
		uint8_t i;
		int16_t START_ID, message_ID, len;
		
		write_To_Port(START_SIGN,2);//start sign
		write_To_Port(8,2); 		//length of payload
		write_To_Port(OFFSET_ID,2); //tell the GCS that I want them sweet sweet offsets.
		write_To_Port(0x01,2);

		delay(1000);//wait 1 second for the data to come in
		if(Serial1.available())
		{
			START_ID = Serial1.read()|int16_t(Serial1.read()<<8); //start sign
			if(START_ID == START_SIGN)
			{
				len = Serial1.read()|int16_t(Serial1.read()<<8); //length of packet 40 bytes
				message_ID = Serial1.read()|int16_t(Serial1.read()<<8);
				Serial1.read()|int16_t(Serial1.read()<<8);//waste
				if(message_ID==OFFSET_ID && len == 28)//confirm that you are getting the offsets and nothing else.
				{
					for(i=0;i<3;i++)//computer has offsets
					{
						A[i] = Serial1.read()|int16_t(Serial1.read()<<8);
						G[i] = Serial1.read()|int16_t(Serial1.read()<<8);
						M[i] = Serial1.read()|int16_t(Serial1.read()<<8);
						gain[i] = Serial1.read()|int16_t(Serial1.read()<<8);
					}
					T = Serial1.read()|int16_t(Serial1.read()<<8);
					return 1;
				}
				else
				{
					return 0;
				}
			}
			else
			{
				return 0; //if computer has no offsets
			}
		}
		return 0;
	} //

	void Send_Offsets(int16_t A[3], int16_t G[3], int16_t M[3], int16_t T,int16_t gain[3])
	{
		uint8_t i;
		write_To_Port(START_SIGN,2);
		write_To_Port(28,2);
		write_To_Port(OFFSET_ID,2);
		write_To_Port(0x01,2);//mode
		for(i=0;i<3;i++)
		{
			write_To_Port(A[i],2);
			write_To_Port(G[i],2);
			write_To_Port(M[i],2);
			write_To_Port(gain[i],2);
		}
		write_To_Port(T,2);
	}//42 bytes sent

	void Send_Config(int16_t params[20])
	{
		uint8_t i;
		write_To_Port(START_SIGN,2);
		write_To_Port(28,2);
		write_To_Port(CONFIG_ID,2);
		write_To_Port(0x01,2);//mode
		for(i=0;i<20;i++)
		{
			write_To_Port(params[i],2);
		}
	}

	bool Get_Config(int16_t params[20])
	{
		uint8_t i;
		int16_t START_ID, message_ID, len;
		
		write_To_Port(START_SIGN,2);//start sign
		write_To_Port(8,2); 		//length of payload
		write_To_Port(CONFIG_ID,2); //tell the GCS that I want them sweet sweet configs.
		write_To_Port(0x01,2);

		delay(100);//wait 1 second for the data to come in
		if(Serial1.available())
		{
			START_ID = Serial1.read()|int16_t(Serial1.read()<<8); //start sign
			if(START_ID == START_SIGN)
			{
				len = Serial1.read()|int16_t(Serial1.read()<<8); //length of packet 40 bytes
				message_ID = Serial1.read()|int16_t(Serial1.read()<<8);
				Serial1.read()|int16_t(Serial1.read()<<8);//waste
				if(message_ID==CONFIG_ID && len == 40)//confirm that you are getting the offsets and nothing else.
				{
					for(i=0;i<20;i++)//computer has offsets
					{
						params[i] = Serial1.read()|int16_t(Serial1.read()<<8);
					}
					return 1;
				}
				else
				{
					return 0;
				}
			}
			else
			{
				return 0; //if computer has no offsets
			}
		}
		return 0;
	}

	void Get_WP(float &X, float &Y, float &slope, int16_t &point)
	{
		received_stamp = millis();
		X = float(int16_t(Serial1.read()|int16_t(Serial1.read()<<8) ) )*1e-2; //coordinates transfered wrt to origin, converted 
		Y = float(int16_t(Serial1.read()|int16_t(Serial1.read()<<8) ) )*1e-2; //coordinates transfered wrt to origin, converted 
		slope = float(int16_t(Serial1.read()|int16_t(Serial1.read()<<8) ) )*1e-2;
		point = int16_t(Serial1.read()|int16_t(Serial1.read()<<8) );	
	}

	bool Send_WP(float X, float Y, float slope,int16_t point)
	{
		if(millis() - transmit_stamp > 100)
		{
			transmit_stamp = millis();
			int x = int16_t(X*1e2);
			int y = int16_t(Y*1e2);
			int m = int16_t(slope*1e2);

			write_To_Port(START_SIGN,2);
			write_To_Port(8,2);
			write_To_Port(WP_ID,2);
			write_To_Port(0x01,2);
			write_To_Port(x,2);
			write_To_Port(y,2);
			write_To_Port(m,2);
			write_To_Port(point,2);
			return 1;
		}
		return 0;
	}//10 bytes 

	void Send_Calib_Command(uint8_t id)
	{
		write_To_Port(START_SIGN,2);
		write_To_Port(ACCEL_CAL, 2);
		write_To_Port(id, 2);
		for(int i = 0;i<62;i++)
		{
			Serial1.write(0xFF);
		}
	}//2 bytes

	// void send_heartbeat(); 
	void Send_State(byte mode,double lon, double lat,double gps_lon, double gps_lat, float vel, float heading, float pitch, float roll,float Accel, float opError, float pError, float head_Error, float VelError, float Time, float Hdop, int16_t comp_status)//position(2), speed(1), heading(1), acceleration(1), Position Error
	{
		if(millis() - transmit_stamp > 100)
		{
			transmit_stamp = millis();
			long out[15];
			out[0] = lon*1e3;
			out[1] = lat*1e3;//hopefully this is correct
			out[2] = gps_lon*1e7;
			out[3] = gps_lat*1e7;
			out[4] = vel*1e2;
			out[5] = heading*1e2;
			out[6] = pitch*1e2;
			out[7] = roll*1e2;
			out[8] = Accel*1e2;
			out[9] = opError*1e3;
			out[10] = pError*1e3;
			out[11] = head_Error*1e3;
			out[12] = VelError*1e3;
			out[13] = Time;
			out[14] = int32_t(Hdop*1e3)<<16|int32_t(comp_status);

			write_To_Port(START_SIGN,2);
			write_To_Port(68,2);
			write_To_Port(STATE_ID,2);
			write_To_Port(int16_t(mode),2);
			for(int i=0;i<15;i++)
			{
				write_To_Port(out[i],4);
			}
		}
	}// 30 bytes

	void Send_Data(const float ekf_states[22],const float A[3], const float G[3], const float M[3],const float AHRS[5],const double gps_lat, const double gps_lon, const float hMSL, const float VelNED[3], const float hAcc, const float vAcc, const float sAcc, const uint8_t fixType, const float alt, const float airspeed, const float flowData[7],const float RngFinderDist, const long max_time)
	{
		if(millis() - transmit_ins_state > 10)
		{
			transmit_ins_state = millis();

			for(int i =0;i<10;i++)
			{
				((float*)(&ins_states))[i] = ekf_states[i];
			}
			for(int i=0;i<3;i++)
			{
				((float*)(&ins_states))[i+10] = AHRS[i];
			}
			for(int i=0;i<4;i++)
			{
				ins_states.Covariance[i] = 10;
			}
			ins_states.max_time = uint16_t(max_time);
			ins_states.UID[0] = 0x00;
			ins_states.UID[1] = 0x01;

			write_To_Port(START_SIGN,2);
			write_To_Port(STATE_ID,2);
			for(int i=0;i<sizeof(ins_states);i++)
			{
				Serial1.write(((unsigned char*)(&ins_states))[i]);
			}
			return; // return so that we don't send both sensor data and state data in the same go
		}
		if(millis() - transmit_sensor_data > 10 and millis()-transmit_ins_state>=3 and millis()-transmit_ins_state<=7) 
		{
			transmit_sensor_data = millis();
			
			sensor_states.lat = long(1e7*gps_lat);
			sensor_states.lon = long(1e7*gps_lon);
			sensor_states.hMSL = long(1e3*hMSL);
			sensor_states.altitude = long(1e3*alt);
			sensor_states.RngFinderDist = long(1e3*RngFinderDist);
			
			sensor_states.gpsAcc[0] = int16_t(1e2*hAcc);
			sensor_states.gpsAcc[1] = int16_t(1e2*vAcc);
			sensor_states.gpsAcc[2] = int16_t(1e2*sAcc);
			sensor_states.airspeed = int16_t(1e2*airspeed);
			
			for(int i=0;i<3;i++)
			{
				sensor_states.VelNED[i] = int16_t(1e2*VelNED[i]);
			}

			for(int i=0;i<3;i++)
			{
				sensor_states.A[i] = int16_t(1e2*A[i]);
				sensor_states.G[i] = int16_t(1e2*G[i]);
				sensor_states.M[i] = int16_t(1e2*M[i]);
			}
			
			for(int i=0;i<4;i++)
			{
				sensor_states.OPFlow[i] = int8_t(100*flowData[i]);
			}

			write_To_Port(START_SIGN,2);
			write_To_Port(SENSOR_ID,2);
			for(int i=0;i<sizeof(sensor_states);i++)
			{
				Serial1.write(((unsigned char*)(&sensor_states))[i]);
			}
			return; // return so that we don't send both sensor data and state data in the same go
		}
	}

	void get_velpos()
	{
		for(int i=0;i<16;i++)
		{
			((char *)(&external_inputs))[i] = Serial1.read();
		}
	}

	uint16_t check()
	{
		uint16_t START_ID,message_ID;

		if(millis() - received_stamp > 10 or Serial1.available()>=6) //100 Hz 
		{
			received_stamp = millis();
			if(Serial1.available())
			{
				failsafe_stamp = millis();
				START_ID = Serial1.read()|int16_t(Serial1.read()<<8); //start sign
				if(START_ID == START_SIGN)
				{
					msg_len = Serial1.read()|int16_t(Serial1.read()<<8); //length of packet
					message_ID = Serial1.read()|int16_t(Serial1.read()<<8);	
					mode = Serial1.read()|int16_t(Serial1.read()<<8); 
					failsafe = false;
					return message_ID;
				}
				else
				{
					while(Serial1.available())
					{
						Serial1.read();
					}
				}
			}
			if(millis() - failsafe_stamp > 1000)
			{
				failsafe = true;
			}
		}
		return 0xFF;//no message
	}

	uint8_t get_Mode()
	{
		return mode;
	}
	#if 0
	bool wifi_failsafe;
	uint16_t wifi_mode;
	WiFiClient client;
	long wifi_received_stamp, wifi_failsafe_stamp, wifi_transmit_stamp;
	void set_client(WiFiClient &_client)
	{
		client = _client;
		received_stamp = millis();
		wifi_failsafe_stamp = millis();
		wifi_transmit_stamp = millis();
		wifi_failsafe = false;
		wifi_mode = MODE_STANDBY;
	}
	void wifi_To_Port(int32_t a, int bytes)
	{
		uint8_t b[4];
		for(int i = 0;i<bytes;i++)
		{
			b[i] = a>>(8*(i)); //last 8 bits
			client.write(b[i]);
		}
	}
	int16_t wifi_read16()
	{
		int16_t out=0;
		for(int i =0;i<2;i++)
		{
			((char *)(&out))[1-i] = client.read();
		}
		return out;
	}
	int32_t wifi_read32()
	{
		int32_t out=0;
		for(int i=0;i<4;i++)
		{
			((char*)(&out))[3-i] = client.read();
		}
		return out;
	}
	void wifi_send_state()
	{
		wifi_To_Port(START_SIGN,2);
		wifi_To_Port(STATE_ID,2);
		for(int i=0;i<sizeof(ins_states);i++)
		{
			client.write(((unsigned char*)(&ins_states))[i]);
		}

		return; // return so that we don't send both sensor data and state data in the same go
	}
	uint16_t wifi_check()
	{
		uint16_t START_ID,message_ID, wifi_msg_len;

		if(millis() - wifi_received_stamp > 100 or client.available()>=6) //10 Hz 
		{
			wifi_received_stamp = millis();
			if(client.available())
			{
				failsafe_stamp = millis();
				START_ID = uint16_t(wifi_read16());//start sign
				if(START_ID == START_SIGN)
				{
					wifi_msg_len = uint16_t(wifi_read16());//length of packet
					message_ID = uint16_t(wifi_read16());
					wifi_mode = uint16_t(wifi_read16());
					wifi_failsafe = false;
					return message_ID;
				}
				else
				{
					while(client.available())
					{
						client.read();
					}
				}
			}
			if(millis() - wifi_failsafe_stamp > 1000)
			{
				wifi_failsafe = true;
			}
		}
		return 0xFF;//no message
	}
	uint8_t wifi_get_mode()
	{
		return wifi_mode;
	}
	#endif



};

#endif
