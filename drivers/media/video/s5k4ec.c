 #include "generic_sensor.h"

/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*/
static int version = KERNEL_VERSION(0,0,1);
module_param(version, int, S_IRUGO);

int s5k4ec_io=1;

static int debug; 
module_param(debug, int, S_IRUGO|S_IWUSR);

//#define S5K4EC_IO
#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_S5K4EC
#define SENSOR_V4L2_IDENT V4L2_IDENT_S5K4EC
#define SENSOR_ID 0x4EC0
#define SENSOR_BUS_PARAM                     (V4L2_MBUS_MASTER |\
												 V4L2_MBUS_PCLK_SAMPLE_RISING|V4L2_MBUS_HSYNC_ACTIVE_HIGH| V4L2_MBUS_VSYNC_ACTIVE_HIGH|\
												 V4L2_MBUS_DATA_ACTIVE_HIGH  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W					 800
#define SENSOR_PREVIEW_H					 600   //720   //600
#define SENSOR_PREVIEW_FPS					 15000	   // 15fps 
#define SENSOR_FULLRES_L_FPS				 5000	   // 5fps
#define SENSOR_FULLRES_H_FPS				 7500	   // 7.5fps
#define SENSOR_720P_FPS 					 30000
#define SENSOR_1080P_FPS					 15000

#define SENSOR_REGISTER_LEN 				 2		   // sensor register address bytes
#define SENSOR_VALUE_LEN					 2		   // sensor register value bytes
									
/*static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect
                                           |CFG_Scene|CFG_Focus
                                           |CFG_FocusZone);*/

static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect
                                           |CFG_Scene
                                           |CFG_FocusZone);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

#if 0//CONFIG_SENSOR_Focus
#include "ov5640_af_firmware.c"
/* ov5640 VCM Command and Status Registers */
#define CONFIG_SENSOR_FocusCenterInCapture	  0
#define CMD_MAIN_Reg		0x3022
//#define CMD_TAG_Reg			0x3023
#define CMD_ACK_Reg 		0x3023
#define CMD_PARA0_Reg		0x3024
#define CMD_PARA1_Reg		0x3025
#define CMD_PARA2_Reg		0x3026
#define CMD_PARA3_Reg		0x3027
#define CMD_PARA4_Reg		0x3028

//#define STA_ZONE_Reg			0x3026
#define STA_FOCUS_Reg		0x3029

/* ov5640 VCM Command  */

#define ConstFocus_Cmd	  0x04
#define StepMode_Cmd	  0x05
#define PauseFocus_Cmd	  0x06
#define ReturnIdle_Cmd	  0x08
#define SetZone_Cmd 	  0x10
#define UpdateZone_Cmd	  0x12
#define SetMotor_Cmd	  0x20
#define SingleFocus_Cmd 			0x03
#define GetFocusResult_Cmd			0x07
#define ReleaseFocus_Cmd			0x08
#define ZoneRelaunch_Cmd			0x12
#define DefaultZoneConfig_Cmd		0x80
#define TouchZoneConfig_Cmd 		0x81
#define CustomZoneConfig_Cmd		0x8f


/* ov5640 Focus State */
//#define S_FIRWRE				0xFF		/*Firmware is downloaded and not run*/
#define S_STARTUP			0x7e		/*Firmware is initializing*/
#define S_ERROR 			0x7f
#define S_IDLE				0x70		/*Idle state, focus is released; lens is located at the furthest position.*/
#define S_FOCUSING			0x00		/*Auto Focus is running.*/
#define S_FOCUSED			0x10		/*Auto Focus is completed.*/

#define S_CAPTURE			0x12
#define S_STEP					0x20

/* ov5640 Zone State */
#define Zone_Is_Focused(a, zone_val)	(zone_val&(1<<(a-3)))
#define Zone_Get_ID(zone_val)			(zone_val&0x03)

#define Zone_CenterMode   0x01
#define Zone_5xMode 	  0x02
#define Zone_5PlusMode	  0x03
#define Zone_4fMode 	  0x04

#define ZoneSel_Auto	  0x0b
#define ZoneSel_SemiAuto  0x0c
#define ZoneSel_Manual	  0x0d
#define ZoneSel_Rotate	  0x0e

/* ov5640 Step Focus Commands */
#define StepFocus_Near_Tag		 0x01
#define StepFocus_Far_Tag		 0x02
#define StepFocus_Furthest_Tag	 0x03
#define StepFocus_Nearest_Tag	 0x04
#define StepFocus_Spec_Tag		 0x10
#endif

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;
	unsigned short int preview_maxlines;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :				Sensor initial setting;
*  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :			Sensor software reset register;
*  sensor_check_id_data :			Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p: 					Sensor 720p setting, it is for video;
*  sensor_1080p:					Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] ={
{0xfcfc, 0xd000},
	{0x0010, 0x0001},//S/W Reset                                              
	{0x1030, 0x0000},//contint_host_int                                       
	{0x0014, 0x0001},//sw_load_complete - Release CORE (Arm) from reset state 
	{0xffff, 0x000a},//Delay 10ms
    //==================================================================================
    //02.ETC Setting
    //==================================================================================
	{0x0028, 0xd000},
	{0x002a, 0x1082},//cregs_d0_d4_cd10 //D4[9:8], D3[7:6], D2[5:4], D1[3:2], D0[1:0]                     
	{0x0f12, 0x02aa},//cregs_d5_d9_cd10 //D9[9:8], D8[7:6], D7[5:4], D6[3:2], D5[1:0]                     
	{0x002a, 0x1084},                                                                                     
	{0x0f12, 0x02aa},//cregs_clks_output_cd10 //SDA[11:10], SCL[9:8], PCLK[7:6], VSYNC[3:2], HSYNC[1:0]   
	{0x0f12, 0x02aa},
	{0x0f12, 0x0afa},
    //==================================================================================
    // 03.Analog Setting & ASP Control
    //==================================================================================
	{0x0028, 0xd000},
	{0x002a, 0x007a},                                                                                                                                                                                  
	{0x0f12, 0x0000},                                                                                                                                                                                  
	{0x002a, 0xe406},                                                                                                                                                                                  
	{0x0f12, 0x0092},                                                                                                                                                                                  
	{0x002a, 0xe410},                                                                                                                                                                                  
	{0x0f12, 0x3804}, //[15:8]fadlc_filter_co_b, [7:0]fadlc_filter_co_a                                                                                                                                
	{0x002a, 0xe41a},                                                                                                                                                                                  
	{0x0f12, 0x0010},                                                                                                                                                                                  
	{0x002a, 0xe420},                                                                                                                                                                                  
	{0x0f12, 0x0003}, //adlc_fadlc_filter_refresh                                                                                                                                                      
	{0x0f12, 0x0060}, //adlc_filter_level_diff_threshold                                                                                                                                               
	{0x002a, 0xe42e},                                                                                                                                                                                  
	{0x0f12, 0x0004}, //dithered l-ADLC(4bit)                                                                                                                                                          
	{0x002a, 0xf400},                                                                                                                                                                                  
	{0x0f12, 0x5a3c}, //[15:8]stx_width, [7:0]dstx_width                                                                                                                                               
	{0x0f12, 0x0023}, //[14]binning_test [13]gain_mode [11:12]row_id [10]cfpn_test [9]pd_pix [8]teg_en, [7]adc_res, [6]smp_en, [5]ldb_en, [4]ld_en, [3]clp_en [2]srx_en, [1]dshut_en, [0]dcds_en       
	{0x0f12, 0x8080}, //CDS option                                                                                                                                                                     
	{0x0f12, 0x03af}, //[11:6]rst_mx, [5:0]sig_mx                                                                                                                                                      
	{0x0f12, 0x000a}, //Avg mode                                                                                                                                                                       
	{0x0f12, 0xaa54}, //x1~x1.49:No MS, x1.5~x3.99:MS2, x4~x16:MS4                                                                                                                                     
	{0x0f12, 0x0040}, //RMP option [6]1: RES gain                                                                                                                                                      
	{0x0f12, 0x464e}, //[14]msoff_en, [13:8]off_rst, [7:0]adc_sat                                                                                                                                      
	{0x0f12, 0x0240}, //bist_sig_width_e                                                                                                                                                               
	{0x0f12, 0x0240}, //bist_sig_width_o                                                                                                                                                               
	{0x0f12, 0x0040}, //[9]dbs_bist_en, [8:0]bist_rst_width                                                                                                                                            
	{0x0f12, 0x1000}, //[15]aac_en, [14]GCLK_DIV2_EN, [13:10]dl_cont [9:8]dbs_mode, [7:0]dbs_option                                                                                                    
	{0x0f12, 0x55ff}, //bias [15:12]pix, [11:8]pix_bst [7:4]comp2, [3:0]comp1                                                                                                                          
	{0x0f12, 0xd000}, //[15:8]clp_lvl, [7:0]ref_option, [5]pix_bst_en                                                                                                                                  
	{0x0f12, 0x0010}, //[7:0]monit                                                                                                                                                                     
	{0x0f12, 0x0202}, //[15:8]dbr_tune_tgsl, [7:0]dbr_tune_pix                                                                                                                                         
	{0x0f12, 0x0401}, //[15:8]dbr_tune_ntg, [7:0]dbr_tune_rg                                                                                                                                           
	{0x0f12, 0x0022}, //[15:8]reg_option, [7:4]rosc_tune_ncp, [3:0]rosc_tune_cp                                                                                                                        
	{0x0f12, 0x0088}, //PD [8]inrush_ctrl, [7]fblv, [6]reg_ntg, [5]reg_tgsl, [4]reg_rg, [3]reg_pix, [2]ncp_rosc, [1]cp_rosc, [0]cp                                                                     
	{0x0f12, 0x009f}, //[9]capa_ctrl_en, [8:7]fb_lv, [6:5]dbr_clk_sel, [4:0]cp_capa                                                                                                                    
	{0x0f12, 0x0000}, //[15:0]blst_en_cintr                                                                                                                                                            
	{0x0f12, 0x1800}, //[11]blst_en, [10]rfpn_test, [9]sl_off, [8]tx_off, [7:0]rdv_option                                                                                                              
	{0x0f12, 0x0088}, //[15:0]pmg_reg_tune                                                                                                                                                             
	{0x0f12, 0x0000}, //[15:1]analog_dummy, [0]pd_reg_test                                                                                                                                             
	{0x0f12, 0x2428}, //[13:11]srx_gap1, [10:8]srx_gap0, [7:0]stx_gap                                                                                                                                  
	{0x0f12, 0x0000}, //[0]atx_option                                                                                                                                                                  
	{0x0f12, 0x03ee}, //aig_avg_half                                                                                                                                                                   
	{0x0f12, 0x0000}, //[0]hvs_test_reg                                                                                                                                                                
	{0x0f12, 0x0000}, //[0]dbus_bist_auto                                                                                                                                                              
	{0x0f12, 0x0000}, //[7:0]dbr_option                                                                                                                                                                
	{0x002a, 0xf552},                                                                                                                                                                                  
	{0x0f12, 0x0708}, //[7:0]lat_st, [15:8]lat_width                                                                                                                                                   
	{0x0f12, 0x080c}, //[7:0]hold_st, [15:8]hold_width    
    //=================================================================================
    // 04.Trap and Patch  Driver IC DW9714  //update by Chris 20130326
    //=================================================================================
// Start of Patch data                 
/*	{0x0028, 0x7000},
	{0x002a, 0x3af8},
	{0x0f12, 0xb5f8},
	{0x0f12, 0x4b44},
	{0x0f12, 0x4944},
	{0x0f12, 0x4845},
	{0x0f12, 0x2200},
	{0x0f12, 0xc008},
	{0x0f12, 0x6001},
	{0x0f12, 0x4944},
	{0x0f12, 0x4844},
	{0x0f12, 0x2401},
	{0x0f12, 0xf000},
	{0x0f12, 0xfca4},
	{0x0f12, 0x4943},
	{0x0f12, 0x4844},
	{0x0f12, 0x2702},
	{0x0f12, 0x0022},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc9e},
	{0x0f12, 0x0260},
	{0x0f12, 0x4c42},
	{0x0f12, 0x8020},
	{0x0f12, 0x2600},
	{0x0f12, 0x8066},
	{0x0f12, 0x4941},
	{0x0f12, 0x4841},
	{0x0f12, 0x6041},
	{0x0f12, 0x4941},
	{0x0f12, 0x4842},
	{0x0f12, 0x003a},
	{0x0f12, 0x2503},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc90},
	{0x0f12, 0x483d},
	{0x0f12, 0x4940},
	{0x0f12, 0x30c0},
	{0x0f12, 0x63c1},
	{0x0f12, 0x4f3b},
	{0x0f12, 0x483f},
	{0x0f12, 0x3f80},
	{0x0f12, 0x6438},
	{0x0f12, 0x483e},
	{0x0f12, 0x493f},
	{0x0f12, 0x6388},
	{0x0f12, 0x002a},
	{0x0f12, 0x493e},
	{0x0f12, 0x483f},
	{0x0f12, 0x2504},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc7f},
	{0x0f12, 0x002a},
	{0x0f12, 0x493d},
	{0x0f12, 0x483e},
	{0x0f12, 0x2505},
	{0x0f12, 0xf000},
	{0x0f12, 0xf8a7},
	{0x0f12, 0x483c},
	{0x0f12, 0x002a},
	{0x0f12, 0x493c},
	{0x0f12, 0x2506},
	{0x0f12, 0x1d80},
	{0x0f12, 0xf000},
	{0x0f12, 0xf8a0},
	{0x0f12, 0x4838},
	{0x0f12, 0x002a},
	{0x0f12, 0x4939},
	{0x0f12, 0x2507},
	{0x0f12, 0x300c},
	{0x0f12, 0xf000},
	{0x0f12, 0xf899},
	{0x0f12, 0x4835},
	{0x0f12, 0x002a},
	{0x0f12, 0x4937},
	{0x0f12, 0x2508},
	{0x0f12, 0x3010},
	{0x0f12, 0xf000},
	{0x0f12, 0xf892},
	{0x0f12, 0x002a},
	{0x0f12, 0x4935},
	{0x0f12, 0x4835},
	{0x0f12, 0x2509},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc5e},
	{0x0f12, 0x002a},
	{0x0f12, 0x4934},
	{0x0f12, 0x4834},
	{0x0f12, 0x250a},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc58},
	{0x0f12, 0x002a},
	{0x0f12, 0x4933},
	{0x0f12, 0x4833},
	{0x0f12, 0x250b},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc52},
	{0x0f12, 0x002a},
	{0x0f12, 0x4932},
	{0x0f12, 0x4832},
	{0x0f12, 0x250c},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc4c},
	{0x0f12, 0x002a},
	{0x0f12, 0x4931},
	{0x0f12, 0x4831},
	{0x0f12, 0x250d},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc46},
	{0x0f12, 0x002a},
	{0x0f12, 0x4930},
	{0x0f12, 0x4830},
	{0x0f12, 0x250e},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc40},
	{0x0f12, 0x002a},
	{0x0f12, 0x492f},
	{0x0f12, 0x482f},
	{0x0f12, 0x250f},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc3a},
	{0x0f12, 0x8626},
	{0x0f12, 0x20ff},
	{0x0f12, 0x1c40},
	{0x0f12, 0x8660},
	{0x0f12, 0x482c},
	{0x0f12, 0x64f8},
	{0x0f12, 0x492c},
	{0x0f12, 0x482d},
	{0x0f12, 0x2410},
	{0x0f12, 0x002a},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc2e},
	{0x0f12, 0x492b},
	{0x0f12, 0x482c},
	{0x0f12, 0x0022},
	{0x0f12, 0xf000},
	{0x0f12, 0xfc29},
	{0x0f12, 0xbcf8},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0x019c},
	{0x0f12, 0x4ec2},
	{0x0f12, 0x73ff},
	{0x0f12, 0x0000},
	{0x0f12, 0x1f90},
	{0x0f12, 0x7000},
	{0x0f12, 0x3ccd},
	{0x0f12, 0x7000},
	{0x0f12, 0xe38b},
	{0x0f12, 0x0000},
	{0x0f12, 0x3d05},
	{0x0f12, 0x7000},
	{0x0f12, 0xc3b1},
	{0x0f12, 0x0000},
	{0x0f12, 0x4780},
	{0x0f12, 0x7000},
	{0x0f12, 0x3d63},
	{0x0f12, 0x7000},
	{0x0f12, 0x0080},
	{0x0f12, 0x7000},
	{0x0f12, 0x3d9f},
	{0x0f12, 0x7000},
	{0x0f12, 0xb49d},
	{0x0f12, 0x0000},
	{0x0f12, 0x3e4b},
	{0x0f12, 0x7000},
	{0x0f12, 0x3dff},
	{0x0f12, 0x7000},
	{0x0f12, 0xffff},
	{0x0f12, 0x00ff},
	{0x0f12, 0x17e0},
	{0x0f12, 0x7000},
	{0x0f12, 0x3fc7},
	{0x0f12, 0x7000},
	{0x0f12, 0x053d},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a89},
	{0x0f12, 0x6cd2},
	{0x0f12, 0x0000},
	{0x0f12, 0x02c9},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a9a},
	{0x0f12, 0x0000},
	{0x0f12, 0x02d2},
	{0x0f12, 0x4015},
	{0x0f12, 0x7000},
	{0x0f12, 0x9e65},
	{0x0f12, 0x0000},
	{0x0f12, 0x4089},
	{0x0f12, 0x7000},
	{0x0f12, 0x7c49},
	{0x0f12, 0x0000},
	{0x0f12, 0x40fd},
	{0x0f12, 0x7000},
	{0x0f12, 0x7c63},
	{0x0f12, 0x0000},
	{0x0f12, 0x4119},
	{0x0f12, 0x7000},
	{0x0f12, 0x8f01},
	{0x0f12, 0x0000},
	{0x0f12, 0x41bb},
	{0x0f12, 0x7000},
	{0x0f12, 0x7f3f},
	{0x0f12, 0x0000},
	{0x0f12, 0x4249},
	{0x0f12, 0x7000},
	{0x0f12, 0x98c5},
	{0x0f12, 0x0000},
	{0x0f12, 0x43b5},
	{0x0f12, 0x7000},
	{0x0f12, 0x6099},
	{0x0f12, 0x0000},
	{0x0f12, 0x430f},
	{0x0f12, 0x7000},
	{0x0f12, 0x4365},
	{0x0f12, 0x7000},
	{0x0f12, 0xa70b},
	{0x0f12, 0x0000},
	{0x0f12, 0x4387},
	{0x0f12, 0x7000},
	{0x0f12, 0x400d},
	{0x0f12, 0x0000},
	{0x0f12, 0xb570},
	{0x0f12, 0x000c},
	{0x0f12, 0x0015},
	{0x0f12, 0x0029},
	{0x0f12, 0xf000},
	{0x0f12, 0xfbd4},
	{0x0f12, 0x49f8},
	{0x0f12, 0x00a8},
	{0x0f12, 0x500c},
	{0x0f12, 0xbc70},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0x6808},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x6849},
	{0x0f12, 0x0409},
	{0x0f12, 0x0c09},
	{0x0f12, 0x4af3},
	{0x0f12, 0x8992},
	{0x0f12, 0x2a00},
	{0x0f12, 0xd00d},
	{0x0f12, 0x2300},
	{0x0f12, 0x1a89},
	{0x0f12, 0xd400},
	{0x0f12, 0x000b},
	{0x0f12, 0x0419},
	{0x0f12, 0x0c09},
	{0x0f12, 0x23ff},
	{0x0f12, 0x33c1},
	{0x0f12, 0x1810},
	{0x0f12, 0x4298},
	{0x0f12, 0xd800},
	{0x0f12, 0x0003},
	{0x0f12, 0x0418},
	{0x0f12, 0x0c00},
	{0x0f12, 0x4aeb},
	{0x0f12, 0x8150},
	{0x0f12, 0x8191},
	{0x0f12, 0x4770},
	{0x0f12, 0xb5f3},
	{0x0f12, 0x0004},
	{0x0f12, 0xb081},
	{0x0f12, 0x9802},
	{0x0f12, 0x6800},
	{0x0f12, 0x0600},
	{0x0f12, 0x0e00},
	{0x0f12, 0x2201},
	{0x0f12, 0x0015},
	{0x0f12, 0x0021},
	{0x0f12, 0x3910},
	{0x0f12, 0x408a},
	{0x0f12, 0x40a5},
	{0x0f12, 0x4fe4},
	{0x0f12, 0x0016},
	{0x0f12, 0x2c10},
	{0x0f12, 0xda03},
	{0x0f12, 0x8839},
	{0x0f12, 0x43a9},
	{0x0f12, 0x8039},
	{0x0f12, 0xe002},
	{0x0f12, 0x8879},
	{0x0f12, 0x43b1},
	{0x0f12, 0x8079},
	{0x0f12, 0xf000},
	{0x0f12, 0xfba0},
	{0x0f12, 0x2c10},
	{0x0f12, 0xda03},
	{0x0f12, 0x8839},
	{0x0f12, 0x4329},
	{0x0f12, 0x8039},
	{0x0f12, 0xe002},
	{0x0f12, 0x8879},
	{0x0f12, 0x4331},
	{0x0f12, 0x8079},
	{0x0f12, 0x49da},
	{0x0f12, 0x8809},
	{0x0f12, 0x2900},
	{0x0f12, 0xd102},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb99},
	{0x0f12, 0x2000},
	{0x0f12, 0x9902},
	{0x0f12, 0x6008},
	{0x0f12, 0xbcfe},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0xb538},
	{0x0f12, 0x9c04},
	{0x0f12, 0x0015},
	{0x0f12, 0x002a},
	{0x0f12, 0x9400},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb94},
	{0x0f12, 0x4ad1},
	{0x0f12, 0x8811},
	{0x0f12, 0x2900},
	{0x0f12, 0xd00f},
	{0x0f12, 0x8820},
	{0x0f12, 0x4281},
	{0x0f12, 0xd20c},
	{0x0f12, 0x8861},
	{0x0f12, 0x8853},
	{0x0f12, 0x4299},
	{0x0f12, 0xd200},
	{0x0f12, 0x1e40},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x8020},
	{0x0f12, 0x8851},
	{0x0f12, 0x8061},
	{0x0f12, 0x4368},
	{0x0f12, 0x1840},
	{0x0f12, 0x6060},
	{0x0f12, 0xbc38},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0xb5f8},
	{0x0f12, 0x0004},
	{0x0f12, 0x6808},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x2201},
	{0x0f12, 0x0015},
	{0x0f12, 0x0021},
	{0x0f12, 0x3910},
	{0x0f12, 0x408a},
	{0x0f12, 0x40a5},
	{0x0f12, 0x4fbe},
	{0x0f12, 0x0016},
	{0x0f12, 0x2c10},
	{0x0f12, 0xda03},
	{0x0f12, 0x8839},
	{0x0f12, 0x43a9},
	{0x0f12, 0x8039},
	{0x0f12, 0xe002},
	{0x0f12, 0x8879},
	{0x0f12, 0x43b1},
	{0x0f12, 0x8079},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb6d},
	{0x0f12, 0x2c10},
	{0x0f12, 0xda03},
	{0x0f12, 0x8838},
	{0x0f12, 0x4328},
	{0x0f12, 0x8038},
	{0x0f12, 0xe002},
	{0x0f12, 0x8878},
	{0x0f12, 0x4330},
	{0x0f12, 0x8078},
	{0x0f12, 0x48b6},
	{0x0f12, 0x8800},
	{0x0f12, 0x0400},
	{0x0f12, 0xd507},
	{0x0f12, 0x4bb5},
	{0x0f12, 0x7819},
	{0x0f12, 0x4ab5},
	{0x0f12, 0x7810},
	{0x0f12, 0x7018},
	{0x0f12, 0x7011},
	{0x0f12, 0x49b4},
	{0x0f12, 0x8188},
	{0x0f12, 0xbcf8},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0xb538},
	{0x0f12, 0x48b2},
	{0x0f12, 0x4669},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb58},
	{0x0f12, 0x48b1},
	{0x0f12, 0x49b0},
	{0x0f12, 0x69c2},
	{0x0f12, 0x2400},
	{0x0f12, 0x31a8},
	{0x0f12, 0x2a00},
	{0x0f12, 0xd008},
	{0x0f12, 0x61c4},
	{0x0f12, 0x684a},
	{0x0f12, 0x6242},
	{0x0f12, 0x6282},
	{0x0f12, 0x466b},
	{0x0f12, 0x881a},
	{0x0f12, 0x6302},
	{0x0f12, 0x885a},
	{0x0f12, 0x6342},
	{0x0f12, 0x6a02},
	{0x0f12, 0x2a00},
	{0x0f12, 0xd00a},
	{0x0f12, 0x6204},
	{0x0f12, 0x6849},
	{0x0f12, 0x6281},
	{0x0f12, 0x466b},
	{0x0f12, 0x8819},
	{0x0f12, 0x6301},
	{0x0f12, 0x8859},
	{0x0f12, 0x6341},
	{0x0f12, 0x49a5},
	{0x0f12, 0x88c9},
	{0x0f12, 0x63c1},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb40},
	{0x0f12, 0xe7a6},
	{0x0f12, 0xb5f0},
	{0x0f12, 0xb08b},
	{0x0f12, 0x20ff},
	{0x0f12, 0x1c40},
	{0x0f12, 0x49a1},
	{0x0f12, 0x89cc},
	{0x0f12, 0x4e9e},
	{0x0f12, 0x6ab1},
	{0x0f12, 0x4284},
	{0x0f12, 0xd101},
	{0x0f12, 0x489f},
	{0x0f12, 0x6081},
	{0x0f12, 0x6a70},
	{0x0f12, 0x0200},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb37},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x4a96},
	{0x0f12, 0x8a11},
	{0x0f12, 0x9109},
	{0x0f12, 0x2101},
	{0x0f12, 0x0349},
	{0x0f12, 0x4288},
	{0x0f12, 0xd200},
	{0x0f12, 0x0001},
	{0x0f12, 0x4a92},
	{0x0f12, 0x8211},
	{0x0f12, 0x4d97},
	{0x0f12, 0x8829},
	{0x0f12, 0x9108},
	{0x0f12, 0x4a8b},
	{0x0f12, 0x2303},
	{0x0f12, 0x3222},
	{0x0f12, 0x1f91},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb28},
	{0x0f12, 0x8028},
	{0x0f12, 0x488e},
	{0x0f12, 0x4987},
	{0x0f12, 0x6bc2},
	{0x0f12, 0x6ac0},
	{0x0f12, 0x4282},
	{0x0f12, 0xd201},
	{0x0f12, 0x8cc8},
	{0x0f12, 0x8028},
	{0x0f12, 0x88e8},
	{0x0f12, 0x9007},
	{0x0f12, 0x2240},
	{0x0f12, 0x4310},
	{0x0f12, 0x80e8},
	{0x0f12, 0x2000},
	{0x0f12, 0x0041},
	{0x0f12, 0x194b},
	{0x0f12, 0x001e},
	{0x0f12, 0x3680},
	{0x0f12, 0x8bb2},
	{0x0f12, 0xaf04},
	{0x0f12, 0x527a},
	{0x0f12, 0x4a7d},
	{0x0f12, 0x188a},
	{0x0f12, 0x8897},
	{0x0f12, 0x83b7},
	{0x0f12, 0x33a0},
	{0x0f12, 0x891f},
	{0x0f12, 0xae01},
	{0x0f12, 0x5277},
	{0x0f12, 0x8a11},
	{0x0f12, 0x8119},
	{0x0f12, 0x1c40},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x2806},
	{0x0f12, 0xd3e9},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb09},
	{0x0f12, 0xf000},
	{0x0f12, 0xfb0f},
	{0x0f12, 0x4f79},
	{0x0f12, 0x37a8},
	{0x0f12, 0x2800},
	{0x0f12, 0xd10a},
	{0x0f12, 0x1fe0},
	{0x0f12, 0x38fd},
	{0x0f12, 0xd001},
	{0x0f12, 0x1cc0},
	{0x0f12, 0xd105},
	{0x0f12, 0x4874},
	{0x0f12, 0x8829},
	{0x0f12, 0x3818},
	{0x0f12, 0x6840},
	{0x0f12, 0x4348},
	{0x0f12, 0x6078},
	{0x0f12, 0x4972},
	{0x0f12, 0x6878},
	{0x0f12, 0x6b89},
	{0x0f12, 0x4288},
	{0x0f12, 0xd300},
	{0x0f12, 0x0008},
	{0x0f12, 0x6078},
	{0x0f12, 0x2000},
	{0x0f12, 0x0041},
	{0x0f12, 0xaa04},
	{0x0f12, 0x5a53},
	{0x0f12, 0x194a},
	{0x0f12, 0x269c},
	{0x0f12, 0x52b3},
	{0x0f12, 0xab01},
	{0x0f12, 0x5a59},
	{0x0f12, 0x32a0},
	{0x0f12, 0x8111},
	{0x0f12, 0x1c40},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x2806},
	{0x0f12, 0xd3f0},
	{0x0f12, 0x4965},
	{0x0f12, 0x9809},
	{0x0f12, 0x8208},
	{0x0f12, 0x9808},
	{0x0f12, 0x8028},
	{0x0f12, 0x9807},
	{0x0f12, 0x80e8},
	{0x0f12, 0x1fe0},
	{0x0f12, 0x38fd},
	{0x0f12, 0xd13b},
	{0x0f12, 0x4d64},
	{0x0f12, 0x89e8},
	{0x0f12, 0x1fc1},
	{0x0f12, 0x39ff},
	{0x0f12, 0xd136},
	{0x0f12, 0x4c5f},
	{0x0f12, 0x8ae0},
	{0x0f12, 0xf000},
	{0x0f12, 0xfade},
	{0x0f12, 0x0006},
	{0x0f12, 0x8b20},
	{0x0f12, 0xf000},
	{0x0f12, 0xfae2},
	{0x0f12, 0x9000},
	{0x0f12, 0x6aa1},
	{0x0f12, 0x6878},
	{0x0f12, 0x1809},
	{0x0f12, 0x0200},
	{0x0f12, 0xf000},
	{0x0f12, 0xfab5},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x0022},
	{0x0f12, 0x3246},
	{0x0f12, 0x0011},
	{0x0f12, 0x310a},
	{0x0f12, 0x2305},
	{0x0f12, 0xf000},
	{0x0f12, 0xfab2},
	{0x0f12, 0x66e8},
	{0x0f12, 0x6b23},
	{0x0f12, 0x0002},
	{0x0f12, 0x0031},
	{0x0f12, 0x0018},
	{0x0f12, 0xf000},
	{0x0f12, 0xfad3},
	{0x0f12, 0x466b},
	{0x0f12, 0x8518},
	{0x0f12, 0x6eea},
	{0x0f12, 0x6b60},
	{0x0f12, 0x9900},
	{0x0f12, 0xf000},
	{0x0f12, 0xfacc},
	{0x0f12, 0x466b},
	{0x0f12, 0x8558},
	{0x0f12, 0x0029},
	{0x0f12, 0x980a},
	{0x0f12, 0x3170},
	{0x0f12, 0xf000},
	{0x0f12, 0xfacd},
	{0x0f12, 0x0028},
	{0x0f12, 0x3060},
	{0x0f12, 0x8a02},
	{0x0f12, 0x4946},
	{0x0f12, 0x3128},
	{0x0f12, 0x808a},
	{0x0f12, 0x8a42},
	{0x0f12, 0x80ca},
	{0x0f12, 0x8a80},
	{0x0f12, 0x8108},
	{0x0f12, 0xb00b},
	{0x0f12, 0xbcf0},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0xb570},
	{0x0f12, 0x2400},
	{0x0f12, 0x4d46},
	{0x0f12, 0x4846},
	{0x0f12, 0x8881},
	{0x0f12, 0x4846},
	{0x0f12, 0x8041},
	{0x0f12, 0x2101},
	{0x0f12, 0x8001},
	{0x0f12, 0xf000},
	{0x0f12, 0xfabc},
	{0x0f12, 0x4842},
	{0x0f12, 0x3820},
	{0x0f12, 0x8bc0},
	{0x0f12, 0xf000},
	{0x0f12, 0xfabf},
	{0x0f12, 0x4b42},
	{0x0f12, 0x220d},
	{0x0f12, 0x0712},
	{0x0f12, 0x18a8},
	{0x0f12, 0x8806},
	{0x0f12, 0x00e1},
	{0x0f12, 0x18c9},
	{0x0f12, 0x81ce},
	{0x0f12, 0x8846},
	{0x0f12, 0x818e},
	{0x0f12, 0x8886},
	{0x0f12, 0x824e},
	{0x0f12, 0x88c0},
	{0x0f12, 0x8208},
	{0x0f12, 0x3508},
	{0x0f12, 0x042d},
	{0x0f12, 0x0c2d},
	{0x0f12, 0x1c64},
	{0x0f12, 0x0424},
	{0x0f12, 0x0c24},
	{0x0f12, 0x2c07},
	{0x0f12, 0xd3ec},
	{0x0f12, 0xe658},
	{0x0f12, 0xb510},
	{0x0f12, 0x4834},
	{0x0f12, 0x4c34},
	{0x0f12, 0x88c0},
	{0x0f12, 0x8060},
	{0x0f12, 0x2001},
	{0x0f12, 0x8020},
	{0x0f12, 0x4831},
	{0x0f12, 0x3820},
	{0x0f12, 0x8bc0},
	{0x0f12, 0xf000},
	{0x0f12, 0xfa9c},
	{0x0f12, 0x88e0},
	{0x0f12, 0x4a31},
	{0x0f12, 0x2800},
	{0x0f12, 0xd003},
	{0x0f12, 0x4930},
	{0x0f12, 0x8849},
	{0x0f12, 0x2900},
	{0x0f12, 0xd009},
	{0x0f12, 0x2001},
	{0x0f12, 0x03c0},
	{0x0f12, 0x8050},
	{0x0f12, 0x80d0},
	{0x0f12, 0x2000},
	{0x0f12, 0x8090},
	{0x0f12, 0x8110},
	{0x0f12, 0xbc10},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0x8050},
	{0x0f12, 0x8920},
	{0x0f12, 0x80d0},
	{0x0f12, 0x8960},
	{0x0f12, 0x0400},
	{0x0f12, 0x1400},
	{0x0f12, 0x8090},
	{0x0f12, 0x89a1},
	{0x0f12, 0x0409},
	{0x0f12, 0x1409},
	{0x0f12, 0x8111},
	{0x0f12, 0x89e3},
	{0x0f12, 0x8a24},
	{0x0f12, 0x2b00},
	{0x0f12, 0xd104},
	{0x0f12, 0x17c3},
	{0x0f12, 0x0f5b},
	{0x0f12, 0x1818},
	{0x0f12, 0x10c0},
	{0x0f12, 0x8090},
	{0x0f12, 0x2c00},
	{0x0f12, 0xd1e6},
	{0x0f12, 0x17c8},
	{0x0f12, 0x0f40},
	{0x0f12, 0x1840},
	{0x0f12, 0x10c0},
	{0x0f12, 0x8110},
	{0x0f12, 0xe7e0},
	{0x0f12, 0xb510},
	{0x0f12, 0x000c},
	{0x0f12, 0x4919},
	{0x0f12, 0x2204},
	{0x0f12, 0x6820},
	{0x0f12, 0x5e8a},
	{0x0f12, 0x0140},
	{0x0f12, 0x1a80},
	{0x0f12, 0x0280},
	{0x0f12, 0x8849},
	{0x0f12, 0xf000},
	{0x0f12, 0xfa6a},
	{0x0f12, 0x6020},
	{0x0f12, 0xe7d2},
	{0x0f12, 0x38d4},
	{0x0f12, 0x7000},
	{0x0f12, 0x17d0},
	{0x0f12, 0x7000},
	{0x0f12, 0x5000},
	{0x0f12, 0xd000},
	{0x0f12, 0x1100},
	{0x0f12, 0xd000},
	{0x0f12, 0x171a},
	{0x0f12, 0x7000},
	{0x0f12, 0x4780},
	{0x0f12, 0x7000},
	{0x0f12, 0x2fca},
	{0x0f12, 0x7000},
	{0x0f12, 0x2fc5},
	{0x0f12, 0x7000},
	{0x0f12, 0x2fc6},
	{0x0f12, 0x7000},
	{0x0f12, 0x2ed8},
	{0x0f12, 0x7000},
	{0x0f12, 0x2bd0},
	{0x0f12, 0x7000},
	{0x0f12, 0x17e0},
	{0x0f12, 0x7000},
	{0x0f12, 0x2de8},
	{0x0f12, 0x7000},
	{0x0f12, 0x37e0},
	{0x0f12, 0x7000},
	{0x0f12, 0x210c},
	{0x0f12, 0x7000},
	{0x0f12, 0x1484},
	{0x0f12, 0x7000},
	{0x0f12, 0xa006},
	{0x0f12, 0x0000},
	{0x0f12, 0x0724},
	{0x0f12, 0x7000},
	{0x0f12, 0xa000},
	{0x0f12, 0xd000},
	{0x0f12, 0x2270},
	{0x0f12, 0x7000},
	{0x0f12, 0x2558},
	{0x0f12, 0x7000},
	{0x0f12, 0x146c},
	{0x0f12, 0x7000},
	{0x0f12, 0xb510},
	{0x0f12, 0x000c},
	{0x0f12, 0x49c7},
	{0x0f12, 0x2208},
	{0x0f12, 0x6820},
	{0x0f12, 0x5e8a},
	{0x0f12, 0x0140},
	{0x0f12, 0x1a80},
	{0x0f12, 0x0280},
	{0x0f12, 0x88c9},
	{0x0f12, 0xf000},
	{0x0f12, 0xfa30},
	{0x0f12, 0x6020},
	{0x0f12, 0xe798},
	{0x0f12, 0xb5fe},
	{0x0f12, 0x000c},
	{0x0f12, 0x6825},
	{0x0f12, 0x6866},
	{0x0f12, 0x68a0},
	{0x0f12, 0x9001},
	{0x0f12, 0x68e7},
	{0x0f12, 0x1ba8},
	{0x0f12, 0x42b5},
	{0x0f12, 0xda00},
	{0x0f12, 0x1b70},
	{0x0f12, 0x9000},
	{0x0f12, 0x49bb},
	{0x0f12, 0x48bc},
	{0x0f12, 0x884a},
	{0x0f12, 0x8843},
	{0x0f12, 0x435a},
	{0x0f12, 0x2304},
	{0x0f12, 0x5ecb},
	{0x0f12, 0x0a92},
	{0x0f12, 0x18d2},
	{0x0f12, 0x02d2},
	{0x0f12, 0x0c12},
	{0x0f12, 0x88cb},
	{0x0f12, 0x8880},
	{0x0f12, 0x4343},
	{0x0f12, 0x0a98},
	{0x0f12, 0x2308},
	{0x0f12, 0x5ecb},
	{0x0f12, 0x18c0},
	{0x0f12, 0x02c0},
	{0x0f12, 0x0c00},
	{0x0f12, 0x0411},
	{0x0f12, 0x0400},
	{0x0f12, 0x1409},
	{0x0f12, 0x1400},
	{0x0f12, 0x1a08},
	{0x0f12, 0x49b0},
	{0x0f12, 0x39e0},
	{0x0f12, 0x6148},
	{0x0f12, 0x9801},
	{0x0f12, 0x3040},
	{0x0f12, 0x7880},
	{0x0f12, 0x2800},
	{0x0f12, 0xd103},
	{0x0f12, 0x9801},
	{0x0f12, 0x0029},
	{0x0f12, 0xf000},
	{0x0f12, 0xfa03},
	{0x0f12, 0x8839},
	{0x0f12, 0x9800},
	{0x0f12, 0x4281},
	{0x0f12, 0xd814},
	{0x0f12, 0x8879},
	{0x0f12, 0x9800},
	{0x0f12, 0x4281},
	{0x0f12, 0xd20c},
	{0x0f12, 0x9801},
	{0x0f12, 0x0029},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9ff},
	{0x0f12, 0x9801},
	{0x0f12, 0x0029},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9fb},
	{0x0f12, 0x9801},
	{0x0f12, 0x0029},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9f7},
	{0x0f12, 0xe003},
	{0x0f12, 0x9801},
	{0x0f12, 0x0029},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9f2},
	{0x0f12, 0x9801},
	{0x0f12, 0x0032},
	{0x0f12, 0x0039},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9f5},
	{0x0f12, 0x6020},
	{0x0f12, 0xe5d0},
	{0x0f12, 0xb57c},
	{0x0f12, 0x489a},
	{0x0f12, 0xa901},
	{0x0f12, 0x0004},
	{0x0f12, 0xf000},
	{0x0f12, 0xf979},
	{0x0f12, 0x466b},
	{0x0f12, 0x88d9},
	{0x0f12, 0x8898},
	{0x0f12, 0x4b95},
	{0x0f12, 0x3346},
	{0x0f12, 0x1e9a},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9ed},
	{0x0f12, 0x4894},
	{0x0f12, 0x4992},
	{0x0f12, 0x3812},
	{0x0f12, 0x3140},
	{0x0f12, 0x8a42},
	{0x0f12, 0x888b},
	{0x0f12, 0x18d2},
	{0x0f12, 0x8242},
	{0x0f12, 0x8ac2},
	{0x0f12, 0x88c9},
	{0x0f12, 0x1851},
	{0x0f12, 0x82c1},
	{0x0f12, 0x0020},
	{0x0f12, 0x4669},
	{0x0f12, 0xf000},
	{0x0f12, 0xf961},
	{0x0f12, 0x488d},
	{0x0f12, 0x214d},
	{0x0f12, 0x8301},
	{0x0f12, 0x2196},
	{0x0f12, 0x8381},
	{0x0f12, 0x211d},
	{0x0f12, 0x3020},
	{0x0f12, 0x8001},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9db},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9e1},
	{0x0f12, 0x4888},
	{0x0f12, 0x4c88},
	{0x0f12, 0x6e00},
	{0x0f12, 0x60e0},
	{0x0f12, 0x466b},
	{0x0f12, 0x8818},
	{0x0f12, 0x8859},
	{0x0f12, 0x0025},
	{0x0f12, 0x1a40},
	{0x0f12, 0x3540},
	{0x0f12, 0x61a8},
	{0x0f12, 0x487f},
	{0x0f12, 0x9900},
	{0x0f12, 0x3060},
	{0x0f12, 0xf000},
	{0x0f12, 0xf9d9},
	{0x0f12, 0x466b},
	{0x0f12, 0x8819},
	{0x0f12, 0x1de0},
	{0x0f12, 0x30f9},
	{0x0f12, 0x8741},
	{0x0f12, 0x8859},
	{0x0f12, 0x8781},
	{0x0f12, 0x2000},
	{0x0f12, 0x71a0},
	{0x0f12, 0x74a8},
	{0x0f12, 0xbc7c},
	{0x0f12, 0xbc08},
	{0x0f12, 0x4718},
	{0x0f12, 0xb5f8},
	{0x0f12, 0x0005},
	{0x0f12, 0x6808},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x684a},
	{0x0f12, 0x0412},
	{0x0f12, 0x0c12},
	{0x0f12, 0x688e},
	{0x0f12, 0x68cc},
	{0x0f12, 0x4970},
	{0x0f12, 0x884b},
	{0x0f12, 0x4343},
	{0x0f12, 0x0a98},
	{0x0f12, 0x2304},
	{0x0f12, 0x5ecb},
	{0x0f12, 0x18c0},
	{0x0f12, 0x02c0},
	{0x0f12, 0x0c00},
	{0x0f12, 0x88cb},
	{0x0f12, 0x4353},
	{0x0f12, 0x0a9a},
	{0x0f12, 0x2308},
	{0x0f12, 0x5ecb},
	{0x0f12, 0x18d1},
	{0x0f12, 0x02c9},
	{0x0f12, 0x0c09},
	{0x0f12, 0x2701},
	{0x0f12, 0x003a},
	{0x0f12, 0x40aa},
	{0x0f12, 0x9200},
	{0x0f12, 0x002a},
	{0x0f12, 0x3a10},
	{0x0f12, 0x4097},
	{0x0f12, 0x2d10},
	{0x0f12, 0xda06},
	{0x0f12, 0x4a69},
	{0x0f12, 0x9b00},
	{0x0f12, 0x8812},
	{0x0f12, 0x439a},
	{0x0f12, 0x4b67},
	{0x0f12, 0x801a},
	{0x0f12, 0xe003},
	{0x0f12, 0x4b66},
	{0x0f12, 0x885a},
	{0x0f12, 0x43ba},
	{0x0f12, 0x805a},
	{0x0f12, 0x0023},
	{0x0f12, 0x0032},
	{0x0f12, 0xf000},
	{0x0f12, 0xf981},
	{0x0f12, 0x2d10},
	{0x0f12, 0xda05},
	{0x0f12, 0x4961},
	{0x0f12, 0x9a00},
	{0x0f12, 0x8808},
	{0x0f12, 0x4310},
	{0x0f12, 0x8008},
	{0x0f12, 0xe003},
	{0x0f12, 0x485e},
	{0x0f12, 0x8841},
	{0x0f12, 0x4339},
	{0x0f12, 0x8041},
	{0x0f12, 0x4d5b},
	{0x0f12, 0x2000},
	{0x0f12, 0x3580},
	{0x0f12, 0x88aa},
	{0x0f12, 0x5e30},
	{0x0f12, 0x2100},
	{0x0f12, 0xf000},
	{0x0f12, 0xf98d},
	{0x0f12, 0x8030},
	{0x0f12, 0x2000},
	{0x0f12, 0x88aa},
	{0x0f12, 0x5e20},
	{0x0f12, 0x2100},
	{0x0f12, 0xf000},
	{0x0f12, 0xf986},
	{0x0f12, 0x8020},
	{0x0f12, 0xe587},
	{0x0f12, 0xb510},
	{0x0f12, 0xf000},
	{0x0f12, 0xf989},
	{0x0f12, 0x4a53},
	{0x0f12, 0x8d50},
	{0x0f12, 0x2800},
	{0x0f12, 0xd007},
	{0x0f12, 0x494e},
	{0x0f12, 0x31c0},
	{0x0f12, 0x684b},
	{0x0f12, 0x4950},
	{0x0f12, 0x4283},
	{0x0f12, 0xd202},
	{0x0f12, 0x8d90},
	{0x0f12, 0x81c8},
	{0x0f12, 0xe6a0},
	{0x0f12, 0x8dd0},
	{0x0f12, 0x81c8},
	{0x0f12, 0xe69d},
	{0x0f12, 0xb5f8},
	{0x0f12, 0xf000},
	{0x0f12, 0xf97e},
	{0x0f12, 0x4d49},
	{0x0f12, 0x8e28},
	{0x0f12, 0x2800},
	{0x0f12, 0xd01f},
	{0x0f12, 0x4e49},
	{0x0f12, 0x4844},
	{0x0f12, 0x68b4},
	{0x0f12, 0x6800},
	{0x0f12, 0x4284},
	{0x0f12, 0xd903},
	{0x0f12, 0x1a21},
	{0x0f12, 0x0849},
	{0x0f12, 0x1847},
	{0x0f12, 0xe006},
	{0x0f12, 0x4284},
	{0x0f12, 0xd203},
	{0x0f12, 0x1b01},
	{0x0f12, 0x0849},
	{0x0f12, 0x1a47},
	{0x0f12, 0xe000},
	{0x0f12, 0x0027},
	{0x0f12, 0x0020},
	{0x0f12, 0x493b},
	{0x0f12, 0x3120},
	{0x0f12, 0x7a0c},
	{0x0f12, 0x2c00},
	{0x0f12, 0xd004},
	{0x0f12, 0x0200},
	{0x0f12, 0x0039},
	{0x0f12, 0xf000},
	{0x0f12, 0xf8c3},
	{0x0f12, 0x8668},
	{0x0f12, 0x2c00},
	{0x0f12, 0xd000},
	{0x0f12, 0x60b7},
	{0x0f12, 0xe54d},
	{0x0f12, 0x20ff},
	{0x0f12, 0x1c40},
	{0x0f12, 0x8668},
	{0x0f12, 0xe549},
	{0x0f12, 0xb510},
	{0x0f12, 0x000c},
	{0x0f12, 0x6820},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0x4933},
	{0x0f12, 0x8e0a},
	{0x0f12, 0x2a00},
	{0x0f12, 0xd003},
	{0x0f12, 0x8e49},
	{0x0f12, 0x0200},
	{0x0f12, 0xf000},
	{0x0f12, 0xf8ad},
	{0x0f12, 0x6020},
	{0x0f12, 0x0400},
	{0x0f12, 0x0c00},
	{0x0f12, 0xe661},
	{0x0f12, 0xb570},
	{0x0f12, 0x680c},
	{0x0f12, 0x4d2f},
	{0x0f12, 0x0020},
	{0x0f12, 0x6f29},
	{0x0f12, 0xf000},
	{0x0f12, 0xf946},
	{0x0f12, 0x6f69},
	{0x0f12, 0x1d20},
	{0x0f12, 0xf000},
	{0x0f12, 0xf942},
	{0x0f12, 0x4827},
	{0x0f12, 0x8e00},
	{0x0f12, 0x2800},
	{0x0f12, 0xd006},
	{0x0f12, 0x4922},
	{0x0f12, 0x2214},
	{0x0f12, 0x3168},
	{0x0f12, 0x0008},
	{0x0f12, 0x383c},
	{0x0f12, 0xf000},
	{0x0f12, 0xf93f},
	{0x0f12, 0xe488},
	{0x0f12, 0xb5f8},
	{0x0f12, 0x0004},
	{0x0f12, 0x4d24},
	{0x0f12, 0x8b68},
	{0x0f12, 0x2800},
	{0x0f12, 0xd012},
	{0x0f12, 0x4823},
	{0x0f12, 0x8a00},
	{0x0f12, 0x06c0},
	{0x0f12, 0xd50e},
	{0x0f12, 0x4822},
	{0x0f12, 0x7800},
	{0x0f12, 0x2800},
	{0x0f12, 0xd00a},
	{0x0f12, 0x481d},
	{0x0f12, 0x6fc1},
	{0x0f12, 0x2000},
	{0x0f12, 0xf000},
	{0x0f12, 0xf923},
	{0x0f12, 0x8b28},
	{0x0f12, 0x2201},
	{0x0f12, 0x2180},
	{0x0f12, 0xf000},
	{0x0f12, 0xf92c},
	{0x0f12, 0x8328},
	{0x0f12, 0x2101},
	{0x0f12, 0x000d},
	{0x0f12, 0x0020},
	{0x0f12, 0x3810},
	{0x0f12, 0x4081},
	{0x0f12, 0x40a5},
	{0x0f12, 0x4f11},
	{0x0f12, 0x000e},
	{0x0f12, 0x2c10},
	{0x0f12, 0xda03},
	{0x0f12, 0x8838},
	{0x0f12, 0x43a8},
	{0x0f12, 0x8038},
	{0x0f12, 0xe002},
	{0x0f12, 0x8878},
	{0x0f12, 0x43b0},
	{0x0f12, 0x8078},
	{0x0f12, 0xf000},
	{0x0f12, 0xf920},
	{0x0f12, 0x2c10},
	{0x0f12, 0xda03},
	{0x0f12, 0x8838},
	{0x0f12, 0x4328},
	{0x0f12, 0x8038},
	{0x0f12, 0xe4ef},
	{0x0f12, 0x8878},
	{0x0f12, 0x4330},
	{0x0f12, 0x8078},
	{0x0f12, 0xe4eb},
	{0x0f12, 0x2558},
	{0x0f12, 0x7000},
	{0x0f12, 0x2ab8},
	{0x0f12, 0x7000},
	{0x0f12, 0x145e},
	{0x0f12, 0x7000},
	{0x0f12, 0x2698},
	{0x0f12, 0x7000},
	{0x0f12, 0x2bb8},
	{0x0f12, 0x7000},
	{0x0f12, 0x2998},
	{0x0f12, 0x7000},
	{0x0f12, 0x1100},
	{0x0f12, 0xd000},
	{0x0f12, 0x4780},
	{0x0f12, 0x7000},
	{0x0f12, 0xe200},
	{0x0f12, 0xd000},
	{0x0f12, 0x210c},
	{0x0f12, 0x7000},
	{0x0f12, 0x0000},
	{0x0f12, 0x7000},
	{0x0f12, 0x308c},
	{0x0f12, 0x7000},
	{0x0f12, 0xb040},
	{0x0f12, 0xd000},
	{0x0f12, 0x3858},
	{0x0f12, 0x7000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x1789},
	{0x0f12, 0x0001},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x16f1},
	{0x0f12, 0x0001},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xc3b1},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xc36d},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xf6d7},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xb49d},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7edf},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x448d},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xf004},
	{0x0f12, 0xe51f},
	{0x0f12, 0x29ec},
	{0x0f12, 0x0001},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x2ef1},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xee03},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xa58b},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7c49},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7c63},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x2db7},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xeb3d},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xf061},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xf0ef},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xf004},
	{0x0f12, 0xe51f},
	{0x0f12, 0x2824},
	{0x0f12, 0x0001},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x8edd},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x8dcb},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x8e17},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x98c5},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7c7d},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7e31},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7eab},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x7501},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0xf63f},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x3d0b},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x29bf},
	{0x0f12, 0x0001},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xf004},
	{0x0f12, 0xe51f},
	{0x0f12, 0x26d8},
	{0x0f12, 0x0001},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x306b},
	{0x0f12, 0x0000},
	{0x0f12, 0x4778},
	{0x0f12, 0x46c0},
	{0x0f12, 0xc000},
	{0x0f12, 0xe59f},
	{0x0f12, 0xff1c},
	{0x0f12, 0xe12f},
	{0x0f12, 0x6099},
	{0x0f12, 0x0000},
*/
	
	//*************************************************************************///
// 05.  OTP setting																																//
//*************************************************************************///
{0x0028,0x7000},
{0x002A,0x0722},
{0x0F12,0x0100},//#skl_OTP_usWaitTimeThisregshouldbeinforntofD0001000//
{0x002A,0x0726},
{0x0F12,0x0000},//#skl_bUseOTPfuncOTPshadingisused,thisregshouldbe1//
{0x002A,0x08D6},
{0x0F12,0x0000},//#ash_bUseOTPDataOTPshadingisused,thisregshouldbe1//
{0x002A,0x146E},
{0x0F12,0x0001},//#awbb_otp_disableOTPAWB(0:useAWBCal.)//
{0x002A,0x08DC},
{0x0F12,0x0000},//#ash_bUseGasAlphaOTPOTPalphaisused,thisregshouldbe1//

{0x0028,0xD000},
{0x002A,0x1000},
{0x0F12,0x0001},


	
    //================================================================================== 
    // 06.Gas_Anti Shading                                                               	
    //================================================================================== 	

/*
// Refer Mon_AWB_RotGain  
	{0x0028, 0x7000},
	{0x002a, 0x08b4},                                                           
	{0x0f12, 0x0001}, //wbt_bUseOutdoorASH                                      
	{0x002a, 0x08bc},                                                           
	{0x0f12, 0x00c0}, //TVAR_ash_AwbAshCord_0_ 2300K                            
	{0x0f12, 0x00df}, //TVAR_ash_AwbAshCord_1_ 2750K                            
	{0x0f12, 0x0100}, //TVAR_ash_AwbAshCord_2_ 3300K                            
	{0x0f12, 0x0125}, //TVAR_ash_AwbAshCord_3_ 4150K                            
	{0x0f12, 0x015f}, //TVAR_ash_AwbAshCord_4_ 5250K                            
	{0x0f12, 0x017c}, //TVAR_ash_AwbAshCord_5_ 6400K                            
	{0x0f12, 0x0194}, //TVAR_ash_AwbAshCord_6_ 7500K                            
	{0x002a, 0x08f6},                                                           
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_0__0_ R  // 2300K                     
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_0__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_0__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_0__3_ B                               
	{0x0f12, 0x4000}, //20130513, 4000, //TVAR_ash_GASAlpha_1__0_ R  // 2750K   
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_1__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_1__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_1__3_ B                               
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_2__0_ R  // 3300K                     
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_2__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_2__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_2__3_ B                               
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_3__0_ R  // 4150K                     
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_3__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_3__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_3__3_ B                               
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_4__0_ R  // 5250K                     
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_4__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_4__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_4__3_ B                               
	{0x0f12, 0x4300}, //TVAR_ash_GASAlpha_5__0_ R  // 6400K                     
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_5__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_5__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_5__3_ B                               
	{0x0f12, 0x4300}, //TVAR_ash_GASAlpha_6__0_ R  // 7500K                     
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_6__1_ GR                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_6__2_ GB                              
	{0x0f12, 0x4000}, //TVAR_ash_GASAlpha_6__3_ B   
	//Outdoor GAS Alpha   
	{0x0f12, 0x4500},
	{0x0f12, 0x4000},
	{0x0f12, 0x4000},
	{0x0f12, 0x4000},
	
		{0x002A, 0x08F4},
		{0x0F12, 0x0001},//ash_bUseGasAlpha
	*/
	


    //==================================================================================
    // 07. Analog Setting 2
    //==================================================================================
    
        //This register is for FACTORY ONLY.
        //If you change it without prior notification
        //YOU are RESPONSIBLE for the FAILURE that will happen in the future
        //For subsampling Size
	{0x0028, 0x7000},
	{0x002a, 0x18bc},                                                                   
	{0x0f12, 0x0004},                                                                   
	{0x0f12, 0x05b6},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0001},                                                                   
	{0x0f12, 0x05ba},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0007},                                                                   
	{0x0f12, 0x05ba},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x01f4},                                                                   
	{0x0f12, 0x024e},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x01f4},                                                                   
	{0x0f12, 0x05b6},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x01f4},                                                                   
	{0x0f12, 0x05ba},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x01f4},                                                                   
	{0x0f12, 0x024f},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0075},                                                                   
	{0x0f12, 0x00cf},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0075},                                                                   
	{0x0f12, 0x00d6},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0004},                                                                   
	{0x0f12, 0x01f4},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x00f0},                                                                   
	{0x0f12, 0x01f4},                                                                   
	{0x0f12, 0x029e},                                                                   
	{0x0f12, 0x05b2},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x01f8},                                                                   
	{0x0f12, 0x0228},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0208},                                                                   
	{0x0f12, 0x0238},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0218},                                                                   
	{0x0f12, 0x0238},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0001},                                                                   
	{0x0f12, 0x0009},                                                                   
	{0x0f12, 0x00de},                                                                   
	{0x0f12, 0x05c0},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x00df},                                                                   
	{0x0f12, 0x00e4},                                                                   
	{0x0f12, 0x01f8},                                                                   
	{0x0f12, 0x01fd},                                                                   
	{0x0f12, 0x05b6},                                                                   
	{0x0f12, 0x05bb},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x01f8},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0077},                                                                   
	{0x0f12, 0x007e},                                                                   
	{0x0f12, 0x024f},                                                                   
	{0x0f12, 0x025e},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0004},                                                                   
	{0x0f12, 0x09d1},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0001},                                                                   
	{0x0f12, 0x09d5},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0008},                                                                   
	{0x0f12, 0x09d5},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02aa},                                                                   
	{0x0f12, 0x0326},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02aa},                                                                   
	{0x0f12, 0x09d1},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02aa},                                                                   
	{0x0f12, 0x09d5},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02aa},                                                                   
	{0x0f12, 0x0327},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0008},                                                                   
	{0x0f12, 0x0084},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0008},                                                                   
	{0x0f12, 0x008d},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0008},                                                                   
	{0x0f12, 0x02aa},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x00aa},                                                                   
	{0x0f12, 0x02aa},                                                                   
	{0x0f12, 0x03ad},                                                                   
	{0x0f12, 0x09cd},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02ae},                                                                   
	{0x0f12, 0x02de},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02be},                                                                   
	{0x0f12, 0x02ee},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02ce},                                                                   
	{0x0f12, 0x02ee},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0001},                                                                   
	{0x0f12, 0x0009},                                                                   
	{0x0f12, 0x0095},                                                                   
	{0x0f12, 0x09db},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0096},                                                                   
	{0x0f12, 0x009b},                                                                   
	{0x0f12, 0x02ae},                                                                   
	{0x0f12, 0x02b3},                                                                   
	{0x0f12, 0x09d1},                                                                   
	{0x0f12, 0x09d6},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x02ae},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0009},                                                                   
	{0x0f12, 0x0010},                                                                   
	{0x0f12, 0x0327},                                                                   
	{0x0f12, 0x0336},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x002a, 0x1af8},                                                                   
	{0x0f12, 0x5a3c}, //senHal_TuneStr_AngTuneData1_2_D000F400 register at subsampling  
	{0x002a, 0x1896},                                                                   
	{0x0f12, 0x0002}, //senHal_SamplingType 0002 03EE: PLA setting                      
	{0x0f12, 0x0000}, //senHal_SamplingMode 0 : 2 PLA / 1 : 4PLA                        
	{0x0f12, 0x0003}, //senHal_PLAOption  [0] VPLA enable  [1] HPLA enable              
	{0x002a, 0x1b00},                                                                   
	{0x0f12, 0xf428},                                                                   
	{0x0f12, 0xffff},                                                                   
	{0x0f12, 0x0000},                                                                   
	{0x002a, 0x189e},                                                                   
	{0x0f12, 0x0fb0}, //senHal_ExpMinPixels                                             
	{0x002a, 0x18ac},                                                                   
	{0x0f12, 0x0060},   //senHal_uAddColsBin                                            
	{0x0f12, 0x0060}, //senHal_uAddColsNoBin                                            
	{0x0f12, 0x05c0}, //senHal_uMinColsBin                                              
	{0x0f12, 0x05c0}, //senHal_uMinColsNoBin                                            
	{0x002a, 0x1aea},                                                                   
	{0x0f12, 0x8080}, //senHal_SubF404Tune                                              
	{0x0f12, 0x0080}, //senHal_FullF404Tune                                             
	{0x002a, 0x1ae0},                                                                   
	{0x0f12, 0x0000}, //senHal_bSenAAC                                                  
	{0x002a, 0x1a72},                                                                   
	{0x0f12, 0x0000}, //senHal_bSRX SRX off                                             
	{0x002a, 0x18a2},                                                                   
	{0x0f12, 0x0004}, //senHal_NExpLinesCheckFine extend Forbidden area line            
	{0x002a, 0x1a6a},                                                                   
	{0x0f12, 0x009a}, //senHal_usForbiddenRightOfs extend right Forbidden area line     
	{0x002a, 0x385e},                                                                   
	{0x0f12, 0x024c}, //Mon_Sen_uExpPixelsOfs                                           
	{0x002a, 0x0ee6},                                                                   
	{0x0f12, 0x0000}, //setot_bUseDigitalHbin                                           
	{0x002a, 0x1b2a},                                                                   
	{0x0f12, 0x0300}, //70001B2A //senHal_TuneStr2_usAngTuneGainTh                      
	{0x0f12, 0x00d6}, //70001B2C //senHal_TuneStr2_AngTuneF4CA_0_                       
	{0x0f12, 0x008d}, //70001B2E //senHal_TuneStr2_AngTuneF4CA_1_                       
	{0x0f12, 0x00cf}, //70001B30 //senHal_TuneStr2_AngTuneF4C2_0_                       
	{0x0f12, 0x0084}, //70001B32 //senHal_TuneStr2_AngTuneF4C2_1_                       
	

    //==================================================================================
    // 08.AF Setting
    //==================================================================================
    
//AF interface setting
  {0x002A, 0x01FC},
  {0x0F12, 0x0001}, //REG_TC_IPRM_LedGpio, for Flash control
//s002A1720
//s0F120100 //afd_usFlags, Low voltage AF enable
  {0x0F12, 0x0003}, //REG_TC_IPRM_CM_Init_AfModeType, VCM IIC
  {0x0F12, 0x0000}, //REG_TC_IPRM_CM_Init_PwmConfig1
  {0x002A, 0x0204},
  {0x0F12, 0x0061}, //REG_TC_IPRM_CM_Init_GpioConfig1, AF Enable GPIO 6
  {0x002A, 0x020C},
  {0x0F12, 0x2F0C}, //REG_TC_IPRM_CM_Init_Mi2cBit
  {0x0F12, 0x0190}, //REG_TC_IPRM_CM_Init_Mi2cRateKhz, IIC Speed

//AF Window Settings
  {0x002A, 0x0294},
  {0x0F12, 0x0100}, //REG_TC_AF_FstWinStartX
  {0x0F12, 0x00E3}, //REG_TC_AF_FstWinStartY
  {0x0F12, 0x0200}, //REG_TC_AF_FstWinSizeX
  {0x0F12, 0x0238}, //REG_TC_AF_FstWinSizeY
  {0x0F12, 0x018C}, //REG_TC_AF_ScndWinStartX
  {0x0F12, 0x0166}, //REG_TC_AF_ScndWinStartY
  {0x0F12, 0x00E6}, //REG_TC_AF_ScndWinSizeX
  {0x0F12, 0x0132}, //REG_TC_AF_ScndWinSizeY
  {0x0F12, 0x0001}, //REG_TC_AF_WinSizesUpdated
//2nd search setting
  {0x002A, 0x070E},
  {0x0F12, 0x00C0}, //skl_af_StatOvlpExpFactor
  {0x002A, 0x071E},
  {0x0F12, 0x0000}, //skl_af_bAfStatOff
  {0x002A, 0x163C},
  {0x0F12, 0x0000}, //af_search_usAeStable
  {0x002A, 0x1648},
  {0x0F12, 0x9002}, //af_search_usSingleAfFlags
  {0x002A, 0x1652},
  {0x0F12, 0x0002}, //af_search_usFinePeakCount
  {0x0F12, 0x0000}, //af_search_usFineMaxScale
  {0x002A, 0x15E0},
  {0x0F12, 0x0403}, //af_pos_usFineStepNumSize
  {0x002A, 0x1656},
  {0x0F12, 0x0000}, //af_search_usCapturePolicy
//Peak Threshold
  {0x002A, 0x164C},
  {0x0F12, 0x0003}, //af_search_usMinPeakSamples
  {0x002A, 0x163E},
  {0x0F12, 0x00C0}, //af_search_usPeakThr
  {0x0F12, 0x0080}, //af_search_usPeakThrLow
  {0x002A, 0x47A8},
  {0x0F12, 0x0080}, //TNP, Macro Threshold register
//Home Pos
  {0x002A, 0x15D4},
  {0x0F12, 0x0000}, //af_pos_usHomePos
  {0x0F12, 0xD000}, //af_pos_usLowConfPos
//AF statistics
  {0x002A, 0x169A},
  {0x0F12, 0xFF95}, //af_search_usConfCheckOrder_1_
  {0x002A, 0x166A},
  {0x0F12, 0x0280}, //af_search_usConfThr_4_
  {0x002A, 0x1676},
  {0x0F12, 0x03A0}, //af_search_usConfThr_10_
  {0x0F12, 0x0320}, //af_search_usConfThr_11_
  {0x002A, 0x16BC},
  {0x0F12, 0x0030}, //af_stat_usMinStatVal
  {0x002A, 0x16E0},
  {0x0F12, 0x0060}, //af_scene_usSceneLowNormBrThr
  {0x002A, 0x16D4},
  {0x0F12, 0x0010}, //af_stat_usBpfThresh
//night mode
{0x002A, 0x0638},
{0x0F12, 0x0001},
{0x0F12, 0x0000},
{0x0F12, 0x1478},
{0x0F12, 0x0000},
{0x0F12, 0x1A0A},
{0x0F12, 0x0000},
{0x0F12, 0x6810},
{0x0F12, 0x0000},
{0x0F12, 0x6810},
{0x0F12, 0x0000},
{0x0F12, 0xD020},
{0x0F12, 0x0000},
{0x0F12, 0x0428},
{0x0F12, 0x0001},
{0x0F12, 0x1A80},
{0x0F12, 0x0006},
{0x0F12, 0x1A80},
{0x0F12, 0x0006},
{0x0F12, 0x1A80},
{0x0F12, 0x0006},

//AF Lens Position Table Settings
  {0x002A, 0x15E8},
  {0x0F12, 0x0010}, //af_pos_usTableLastInd
  {0x0F12, 0x0018}, //af_pos_usTable
  {0x0F12, 0x0020}, //af_pos_usTable
  {0x0F12, 0x0028}, //af_pos_usTable
  {0x0F12, 0x0030}, //af_pos_usTable
  {0x0F12, 0x0038}, //af_pos_usTable
  {0x0F12, 0x0040}, //af_pos_usTable
  {0x0F12, 0x0048}, //af_pos_usTable
  {0x0F12, 0x0050}, //af_pos_usTable
  {0x0F12, 0x0058}, //af_pos_usTable
  {0x0F12, 0x0060}, //af_pos_usTable
  {0x0F12, 0x0068}, //af_pos_usTable
  {0x0F12, 0x0070}, //af_pos_usTable
  {0x0F12, 0x0078}, //af_pos_usTable
  {0x0F12, 0x0080}, //af_pos_usTable
  {0x0F12, 0x0088}, //af_pos_usTable
  {0x0F12, 0x0090}, //af_pos_usTable
  {0x0F12, 0x0098}, //af_pos_usTable


//VCM AF driver with PWM/I2C
  {0x002A, 0x1722},
  {0x0F12, 0x8000}, //afd_usParam[0] I2C power down command
  {0x0F12, 0x0006}, //afd_usParam[1] Position Right Shift
  {0x0F12, 0x3FF0}, //afd_usParam[2] I2C Data Mask
  {0x0F12, 0x03E8}, //afd_usParam[3] PWM Period
  {0x0F12, 0x0000}, //afd_usParam[4] PWM Divider
  {0x0F12, 0x0020}, //afd_usParam[5] SlowMotion Delay 4. reduce lens collision noise.
  {0x0F12, 0x0010}, //afd_usParam[6] SlowMotion Threshold
  {0x0F12, 0x0008}, //afd_usParam[7] Signal Shaping
  {0x0F12, 0x0040}, //afd_usParam[8] Signal Shaping level
  {0x0F12, 0x0080}, //afd_usParam[9] Signal Shaping level
  {0x0F12, 0x00C0}, //afd_usParam[10] Signal Shaping level
  {0x0F12, 0x00E0}, //afd_usParam[11] Signal Shaping level
  {0x002A, 0x028C},
  {0x0F12, 0x0003}, //REG_TC_AF_AfCmd
             
//20131208
  {0x002A, 0x1720},
  {0x0F12, 0x0000}, 
	
	
	
	
	
	

	    //==================================================================================
    // 09.AWB-BASIC setting
    //==================================================================================

// AWB init Start point
  {0x002A, 0x145E},
  {0x0F12, 0x0580}, //awbb_GainsInit_0_
  {0x0F12, 0x0428}, //awbb_GainsInit_1_
  {0x0F12, 0x07B0}, //awbb_GainsInit_2_
// AWB Convergence Speed
  {0x0F12, 0x0008}, //awbb_WpFilterMinThr
  {0x0F12, 0x0190}, //awbb_WpFilterMaxThr
  {0x0F12, 0x00A0}, //awbb_WpFilterCoef
  {0x0F12, 0x0004}, //awbb_WpFilterSize
  {0x0F12, 0x0002}, //awbb_GridEnable
  {0x002A, 0x144E},
  {0x0F12, 0x0000}, //awbb_RGainOff
  {0x0F12, 0x0000}, //awbb_BGainOff
  {0x0F12, 0x0000}, //awbb_GGainOff
  {0x0F12, 0x00C2}, //awbb_Alpha_Comp_Mode
  {0x0F12, 0x0002}, //awbb_Rpl_InvalidOutDoor
  {0x0F12, 0x0001}, //awbb_UseGrThrCorr
  {0x0F12, 0x0074}, //awbb_Use_Filters
  {0x0F12, 0x0001}, //awbb_CorrectMinNumPatches
// White Locus
  {0x002A, 0x11F0},
  {0x0F12, 0x012C}, //awbb_IntcR
  {0x0F12, 0x0121}, //awbb_IntcB
  {0x0F12, 0x02DF}, //awbb_GLocusR
  {0x0F12, 0x0314}, //awbb_GLocusB
  {0x002A, 0x120E},
  {0x0F12, 0x0000}, //awbb_MovingScale10
  {0x0F12, 0x05FD}, //awbb_GamutWidthThr1
  {0x0F12, 0x036B}, //awbb_GamutHeightThr1
  {0x0F12, 0x0020}, //awbb_GamutWidthThr2
  {0x0F12, 0x001A}, //awbb_GamutHeightThr2
  {0x002A, 0x1278},
  {0x0F12, 0xFEF7}, //awbb_SCDetectionMap_SEC_StartR_B
  {0x0F12, 0x0021}, //awbb_SCDetectionMap_SEC_StepR_B
  {0x0F12, 0x07D0}, //awbb_SCDetectionMap_SEC_SunnyNB
  {0x0F12, 0x07D0}, //awbb_SCDetectionMap_SEC_StepNB
  {0x0F12, 0x01C8}, //awbb_SCDetectionMap_SEC_LowTempR_B
  {0x0F12, 0x0096}, //awbb_SCDetectionMap_SEC_SunnyNBZone
  {0x0F12, 0x0004}, //awbb_SCDetectionMap_SEC_LowTempR_BZone
  {0x002A, 0x1224},
  {0x0F12, 0x0032}, //awbb_LowBr
  {0x0F12, 0x001E}, //awbb_LowBr_NBzone
  {0x0F12, 0x00E2}, //awbb_YThreshHigh
  {0x0F12, 0x0010}, //awbb_YThreshLow_Norm
  {0x0F12, 0x0002}, //awbb_YThreshLow_Low
  {0x002A, 0x2BA4},
  {0x0F12, 0x0004}, //Mon_AWB_ByPassMode
  {0x002A, 0x11FC},
  {0x0F12, 0x000C}, //awbb_MinNumOfFinalPatches
  {0x002A, 0x1208},
  {0x0F12, 0x0020}, //awbb_MinNumOfChromaclassifpatches
// Indoor Zone
  {0x002A, 0x101C},
  {0x0F12, 0x0360}, //awbb_IndoorGrZones_m_BGrid_0__m_left
  {0x0F12, 0x036C}, //awbb_IndoorGrZones_m_BGrid_0__m_right
  {0x0F12, 0x0320}, //awbb_IndoorGrZones_m_BGrid_1__m_left
  {0x0F12, 0x038A}, //awbb_IndoorGrZones_m_BGrid_1__m_right
  {0x0F12, 0x02E8}, //awbb_IndoorGrZones_m_BGrid_2__m_left
  {0x0F12, 0x0380}, //awbb_IndoorGrZones_m_BGrid_2__m_right
  {0x0F12, 0x02BE}, //awbb_IndoorGrZones_m_BGrid_3__m_left
  {0x0F12, 0x035A}, //awbb_IndoorGrZones_m_BGrid_3__m_right
  {0x0F12, 0x0298}, //awbb_IndoorGrZones_m_BGrid_4__m_left
  {0x0F12, 0x0334}, //awbb_IndoorGrZones_m_BGrid_4__m_right
  {0x0F12, 0x0272}, //awbb_IndoorGrZones_m_BGrid_5__m_left
  {0x0F12, 0x030E}, //awbb_IndoorGrZones_m_BGrid_5__m_right
  {0x0F12, 0x024C}, //awbb_IndoorGrZones_m_BGrid_6__m_left
  {0x0F12, 0x02EA}, //awbb_IndoorGrZones_m_BGrid_6__m_right
  {0x0F12, 0x0230}, //awbb_IndoorGrZones_m_BGrid_7__m_left
  {0x0F12, 0x02CC}, //awbb_IndoorGrZones_m_BGrid_7__m_right
  {0x0F12, 0x0214}, //awbb_IndoorGrZones_m_BGrid_8__m_left
  {0x0F12, 0x02B0}, //awbb_IndoorGrZones_m_BGrid_8__m_right
  {0x0F12, 0x01F8}, //awbb_IndoorGrZones_m_BGrid_9__m_left
  {0x0F12, 0x0294}, //awbb_IndoorGrZones_m_BGrid_9__m_right
  {0x0F12, 0x01DC}, //awbb_IndoorGrZones_m_BGrid_10__m_left
  {0x0F12, 0x0278}, //awbb_IndoorGrZones_m_BGrid_10__m_right
  {0x0F12, 0x01C0}, //awbb_IndoorGrZones_m_BGrid_11__m_left
  {0x0F12, 0x0264}, //awbb_IndoorGrZones_m_BGrid_11__m_right
  {0x0F12, 0x01AA}, //awbb_IndoorGrZones_m_BGrid_12__m_left
  {0x0F12, 0x0250}, //awbb_IndoorGrZones_m_BGrid_12__m_right
  {0x0F12, 0x0196}, //awbb_IndoorGrZones_m_BGrid_13__m_left
  {0x0F12, 0x023C}, //awbb_IndoorGrZones_m_BGrid_13__m_right
  {0x0F12, 0x0180}, //awbb_IndoorGrZones_m_BGrid_14__m_left
  {0x0F12, 0x0228}, //awbb_IndoorGrZones_m_BGrid_14__m_right
  {0x0F12, 0x016C}, //awbb_IndoorGrZones_m_BGrid_15__m_left
  {0x0F12, 0x0214}, //awbb_IndoorGrZones_m_BGrid_15__m_right
  {0x0F12, 0x0168}, //awbb_IndoorGrZones_m_BGrid_16__m_left
  {0x0F12, 0x0200}, //awbb_IndoorGrZones_m_BGrid_16__m_right
  {0x0F12, 0x0172}, //awbb_IndoorGrZones_m_BGrid_17__m_left
  {0x0F12, 0x01EC}, //awbb_IndoorGrZones_m_BGrid_17__m_right
  {0x0F12, 0x019A}, //awbb_IndoorGrZones_m_BGrid_18__m_left
  {0x0F12, 0x01D8}, //awbb_IndoorGrZones_m_BGrid_18__m_right
  {0x0F12, 0x0000}, //awbb_IndoorGrZones_m_BGrid_19__m_left
  {0x0F12, 0x0000}, //awbb_IndoorGrZones_m_BGrid_19__m_right
  {0x0F12, 0x0005}, //awbb_IndoorGrZones_m_GridStep
  {0x002A, 0x1070},
  {0x0F12, 0x0013}, //awbb_IndoorGrZones_ZInfo_m_GridSz
  {0x002A, 0x1074},
  {0x0F12, 0x00EC}, //awbb_IndoorGrZones_m_Boffs
// Outdoor Zone
  {0x002A, 0x1078},
  {0x0F12, 0x0232}, //awbb_OutdoorGrZones_m_BGrid_0__m_left
  {0x0F12, 0x025A}, //awbb_OutdoorGrZones_m_BGrid_0__m_right
  {0x0F12, 0x021E}, //awbb_OutdoorGrZones_m_BGrid_1__m_left
  {0x0F12, 0x0274}, //awbb_OutdoorGrZones_m_BGrid_1__m_right
  {0x0F12, 0x020E}, //awbb_OutdoorGrZones_m_BGrid_2__m_left
  {0x0F12, 0x028E}, //awbb_OutdoorGrZones_m_BGrid_2__m_right
  {0x0F12, 0x0200}, //awbb_OutdoorGrZones_m_BGrid_3__m_left
  {0x0F12, 0x0290}, //awbb_OutdoorGrZones_m_BGrid_3__m_right
  {0x0F12, 0x01F4}, //awbb_OutdoorGrZones_m_BGrid_4__m_left
  {0x0F12, 0x0286}, //awbb_OutdoorGrZones_m_BGrid_4__m_right
  {0x0F12, 0x01E8}, //awbb_OutdoorGrZones_m_BGrid_5__m_left
  {0x0F12, 0x027E}, //awbb_OutdoorGrZones_m_BGrid_5__m_right
  {0x0F12, 0x01DE}, //awbb_OutdoorGrZones_m_BGrid_6__m_left
  {0x0F12, 0x0274}, //awbb_OutdoorGrZones_m_BGrid_6__m_right
  {0x0F12, 0x01D2}, //awbb_OutdoorGrZones_m_BGrid_7__m_left
  {0x0F12, 0x0268}, //awbb_OutdoorGrZones_m_BGrid_7__m_right
  {0x0F12, 0x01D0}, //awbb_OutdoorGrZones_m_BGrid_8__m_left
  {0x0F12, 0x025E}, //awbb_OutdoorGrZones_m_BGrid_8__m_right
  {0x0F12, 0x01D6}, //awbb_OutdoorGrZones_m_BGrid_9__m_left
  {0x0F12, 0x0252}, //awbb_OutdoorGrZones_m_BGrid_9__m_right
  {0x0F12, 0x01E2}, //awbb_OutdoorGrZones_m_BGrid_10__m_left
  {0x0F12, 0x0248}, //awbb_OutdoorGrZones_m_BGrid_10__m_right
  {0x0F12, 0x01F4}, //awbb_OutdoorGrZones_m_BGrid_11__m_left
  {0x0F12, 0x021A}, //awbb_OutdoorGrZones_m_BGrid_11__m_right
  {0x0F12, 0x0004}, //awbb_OutdoorGrZones_m_GridStep
  {0x002A, 0x10AC},
  {0x0F12, 0x000C}, //awbb_OutdoorGrZones_ZInfo_m_GridSz
  {0x002A, 0x10B0},
  {0x0F12, 0x01DA}, //awbb_OutdoorGrZones_m_Boffs
// Low Brightness Zone
  {0x002A, 0x10B4},
  {0x0F12, 0x0348}, //awbb_LowBrGrZones_m_BGrid_0__m_left
  {0x0F12, 0x03B6}, //awbb_LowBrGrZones_m_BGrid_0__m_right
  {0x0F12, 0x02B8}, //awbb_LowBrGrZones_m_BGrid_1__m_left
  {0x0F12, 0x03B6}, //awbb_LowBrGrZones_m_BGrid_1__m_right
  {0x0F12, 0x0258}, //awbb_LowBrGrZones_m_BGrid_2__m_left
  {0x0F12, 0x038E}, //awbb_LowBrGrZones_m_BGrid_2__m_right
  {0x0F12, 0x0212}, //awbb_LowBrGrZones_m_BGrid_3__m_left
  {0x0F12, 0x0348}, //awbb_LowBrGrZones_m_BGrid_3__m_right
  {0x0F12, 0x01CC}, //awbb_LowBrGrZones_m_BGrid_4__m_left
  {0x0F12, 0x030C}, //awbb_LowBrGrZones_m_BGrid_4__m_right
  {0x0F12, 0x01A2}, //awbb_LowBrGrZones_m_BGrid_5__m_left
  {0x0F12, 0x02D2}, //awbb_LowBrGrZones_m_BGrid_5__m_right
  {0x0F12, 0x0170}, //awbb_LowBrGrZones_m_BGrid_6__m_left
  {0x0F12, 0x02A6}, //awbb_LowBrGrZones_m_BGrid_6__m_right
  {0x0F12, 0x014C}, //awbb_LowBrGrZones_m_BGrid_7__m_left
  {0x0F12, 0x0280}, //awbb_LowBrGrZones_m_BGrid_7__m_right
  {0x0F12, 0x0128}, //awbb_LowBrGrZones_m_BGrid_8__m_left
  {0x0F12, 0x025C}, //awbb_LowBrGrZones_m_BGrid_8__m_right
  {0x0F12, 0x0146}, //awbb_LowBrGrZones_m_BGrid_9__m_left
  {0x0F12, 0x0236}, //awbb_LowBrGrZones_m_BGrid_9__m_right
  {0x0F12, 0x0164}, //awbb_LowBrGrZones_m_BGrid_10__m_left
  {0x0F12, 0x0212}, //awbb_LowBrGrZones_m_BGrid_10__m_right
  {0x0F12, 0x0000}, //awbb_LowBrGrZones_m_BGrid_11__m_left
  {0x0F12, 0x0000}, //awbb_LowBrGrZones_m_BGrid_11__m_right
  {0x0F12, 0x0006}, //awbb_LowBrGrZones_m_GridStep
  {0x002A, 0x10E8},
  {0x0F12, 0x000B}, //awbb_LowBrGrZones_ZInfo_m_GridSz
  {0x002A, 0x10EC},
  {0x0F12, 0x00D2}, //awbb_LowBrGrZones_m_Boffs

// Low Temp. Zone
  {0x002A, 0x10F0},
  {0x0F12, 0x039A},
  {0x0F12, 0x0000}, //awbb_CrclLowT_R_c
  {0x0F12, 0x00FE},
  {0x0F12, 0x0000}, //awbb_CrclLowT_B_c
  {0x0F12, 0x2284},
  {0x0F12, 0x0000}, //awbb_CrclLowT_Rad_c

//AWB - GridCorrection
  {0x002A, 0x1434},
  {0x0F12, 0x02C1}, //awbb_GridConst_1_0_
  {0x0F12, 0x033A}, //awbb_GridConst_1_1_
  {0x0F12, 0x038A}, //awbb_GridConst_1_2_
  {0x0F12, 0x101A}, //awbb_GridConst_2_0_
  {0x0F12, 0x1075}, //awbb_GridConst_2_1_
  {0x0F12, 0x113D}, //awbb_GridConst_2_2_
  {0x0F12, 0x113F}, //awbb_GridConst_2_3_
  {0x0F12, 0x11AF}, //awbb_GridConst_2_4_
  {0x0F12, 0x11F0}, //awbb_GridConst_2_5_
  {0x0F12, 0x00B2}, //awbb_GridCoeff_R_1
  {0x0F12, 0x00B8}, //awbb_GridCoeff_B_1
  {0x0F12, 0x00CA}, //awbb_GridCoeff_R_2
  {0x0F12, 0x009D}, //awbb_GridCoeff_B_2

// Indoor Grid Offset
  {0x002A, 0x13A4},
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_2__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_2__5_

// Outdoor Grid Offset
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_R_Out_2__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_0__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_1__5_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__0_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__1_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__2_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__3_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__4_
  {0x0F12, 0x0000}, //awbb_GridCorr_B_Out_2__5_

	
	
    //==================================================================================
    // 10.Clock Setting
    //==================================================================================
    //For MCLK=24MHz, PCLK=5DC0
	{0x002a, 0x01f8},      //System Setting  
	{0x0f12, 0x5dc0},
	{0x002a, 0x0212},//
	{0x0f12, 0x0002},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},//0x0216  PVI
	{0x002a, 0x021a},
	{0x0f12, 0x34bc},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x4f1a},
	{0x0f12, 0x4f1a},
	{0x002a, 0x0f30},
	{0x0f12, 0x0001},
	{0x002a, 0x0f2a},
	{0x0f12, 0x0000},//AFC_Default60Hz 0001:60Hz 0000h:50Hz
	{0x002a, 0x04e6},//REG_TC_DBG_AutoAlgEnBits
	{0x0f12, 0x077f},//0x075f



	
	{0x002a, 0x04d6},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0x002a, 0x1484},
	{0x0f12, 0x0038},//TVAR_ae_BrAve
 //ae_StatMode bit[3] BLC has to be bypassed to prevent AE weight change especially backlight scene
   {0x002A, 0x148A},
   {0x0F12, 0x000F},   //ae_StatMode
   {0x002A, 0x058C},
   {0x0F12, 0x3520},
   {0x0F12, 0x0000},   //lt_uMaxExp1
   {0x0F12, 0xD4C0},
   {0x0F12, 0x0001},   //lt_uMaxExp2
   {0x0F12, 0x3520},
   {0x0F12, 0x0000},   //lt_uCapMaxExp1
   {0x0F12, 0xD4C0},
   {0x0F12, 0x0001},   //lt_uCapMaxExp2
   {0x002A, 0x059C},
   {0x0F12, 0x0470},   //lt_uMaxAnGain1
   {0x0F12, 0x0C00},   //lt_uMaxAnGain2
   {0x0F12, 0x0100},   //lt_uMaxDigGain
   {0x0F12, 0x1000},   //lt_uMaxTotGain
   {0x002A, 0x0544},
   {0x0F12, 0x0111},   //lt_uLimitHigh
   {0x0F12, 0x00EF},   //lt_uLimitLow
   {0x002A, 0x0608},
   {0x0F12, 0x0001},   //lt_ExpGain_uSubsamplingmode
   {0x0F12, 0x0001},   //lt_ExpGain_uNonSubsampling
   {0x0F12, 0x0800},   //lt_ExpGain_ExpCurveGainMaxStr
   {0x0F12, 0x0100},   //0100   //lt_ExpGain_ExpCurveGainMaxStr_0__uMaxDigGain
   {0x0F12, 0x0001},   //0001
   {0x0F12, 0x0000},   //0000   //lt_ExpGain_ExpCurveGainMaxStr_0__ulExpIn_0_
   {0x0F12, 0x0A3C},   //0A3C
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x0D05},   //0D05
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x4008},   //4008
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x7000},   //7400  //?? //700Lux
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x9C00},   //C000  //?? //9C00->9F->A5 //400Lux
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0xAD00},   //AD00
   {0x0F12, 0x0001},   //0001
   {0x0F12, 0xF1D4},   //F1D4
   {0x0F12, 0x0002},   //0002
   {0x0F12, 0xDC00},   //DC00
   {0x0F12, 0x0005},   //0005
   {0x0F12, 0xDC00},   //DC00
   {0x0F12, 0x0005},   //0005         //
   {0x002A, 0x0638},   //0638
   {0x0F12, 0x0001},   //0001
   {0x0F12, 0x0000},   //0000   //lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_0_
   {0x0F12, 0x0A3C},   //0A3C
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x0D05},   //0D05
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x3408},   //3408
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x3408},   //3408
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x6810},   //6810
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0x8214},   //8214
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0xC350},   //C350
   {0x0F12, 0x0000},   //0000
   {0x0F12, 0xD4C0},   //C350
   {0x0F12, 0x0001},   //0000
   {0x0F12, 0xD4C0},   //C350
   {0x0F12, 0x0001},   //0000
   {0x002A, 0x0660},
   {0x0F12, 0x0650},   //lt_ExpGain_ExpCurveGainMaxStr_1_
   {0x0F12, 0x0100},   //lt_ExpGain_ExpCurveGainMaxStr_1__uMaxDigGain
   {0x002A, 0x06B8},
   {0x0F12, 0x452C},
   {0x0F12, 0x000A},   //0005   //lt_uMaxLei
   {0x002A, 0x05D0},
   {0x0F12, 0x0000},   //lt_mbr_Peak_behind 
	
	


	
    //================================================================================== 
    // 13.AE Weight (Normal)                                                             
    //================================================================================== 
	{0x002a, 0x1492},
	{0x0f12, 0x0100},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0001},
	{0x0f12, 0x0101},
	{0x0f12, 0x0201},
	{0x0f12, 0x0102},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0202},
	{0x0f12, 0x0202},
	{0x0f12, 0x0101},
	{0x0f12, 0x0201},
	{0x0f12, 0x0302},
	{0x0f12, 0x0203},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0302},
	{0x0f12, 0x0203},
	{0x0f12, 0x0102},
	{0x0f12, 0x0201},
	{0x0f12, 0x0202},
	{0x0f12, 0x0202},
	{0x0f12, 0x0102},
	{0x0f12, 0x0101},
	{0x0f12, 0x0202},
	{0x0f12, 0x0202},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x0f12, 0x0101},
	{0x002a, 0x0484},
	{0x0f12, 0x0002},
	{0x002a, 0x183a},
	{0x0f12, 0x0001},
	{0x002a, 0x17f6},
	{0x0f12, 0x023c},
	{0x0f12, 0x0248},
	{0x002a, 0x1840},
	{0x0f12, 0x0001},
	{0x0f12, 0x0100},
	{0x0f12, 0x0120},
	{0x0f12, 0x0180},
	{0x0f12, 0x0200},
	{0x0f12, 0x0400},
	{0x0f12, 0x0800},
	{0x0f12, 0x0a00},
	{0x0f12, 0x1000},
	{0x0f12, 0x0100},
	{0x0f12, 0x00a0},
	{0x0f12, 0x0090},
	{0x0f12, 0x0080},
	{0x0f12, 0x0070},
	{0x0f12, 0x0045},
	{0x0f12, 0x0030},
	{0x0f12, 0x0010},
	{0x002a, 0x1884},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x0f12, 0x0100},
	{0x002a, 0x1826},
	{0x0f12, 0x0100},
	{0x0f12, 0x00c0},
	{0x0f12, 0x0080},
	{0x0f12, 0x000a},
	{0x0f12, 0x0000},
	{0x0f12, 0x0030},
	{0x0f12, 0x0040},
	{0x0f12, 0x0048},
	{0x0f12, 0x0050},
	{0x0f12, 0x0060},
	{0x002a, 0x4784},
	{0x0f12, 0x00a0},
	{0x0f12, 0x00c0},
	{0x0f12, 0x00d0},
	{0x0f12, 0x0100},
	{0x0f12, 0x0200},
	{0x0f12, 0x0300},
	{0x0f12, 0x0088},
	{0x0f12, 0x00b0},
	{0x0f12, 0x00c0},
	{0x0f12, 0x0100},
	{0x0f12, 0x0200},
	{0x0f12, 0x0300},
	{0x002a, 0x479c},
	{0x0f12, 0x0120},
	{0x0f12, 0x0150},
	{0x0f12, 0x0200},
	{0x0f12, 0x003c},
	{0x0f12, 0x003b},
	{0x0f12, 0x0026},
	

	

    //==================================================================================
    // 15.CCM Setting
    //==================================================================================
 {0x002A, 0x08A6},
 {0x0F12, 0x00C0}, //SARR_AwbCcmCord[0]
 {0x0F12, 0x0100}, //SARR_AwbCcmCord[1]
 {0x0F12, 0x0125}, //SARR_AwbCcmCord[2]
 {0x0F12, 0x015F}, //SARR_AwbCcmCord[3]
 {0x0F12, 0x017C}, //SARR_AwbCcmCord[4]
 {0x0F12, 0x0194}, //SARR_AwbCcmCord[5]
 {0x002A, 0x0898},
 {0x0F12, 0x4800}, //TVAR_wbt_pBaseCcms
 {0x0F12, 0x7000},
 {0x002A, 0x08A0},
 {0x0F12, 0x48D8}, //TVAR_wbt_pOutdoorCcm
 {0x0F12, 0x7000},
//Horizon
 {0x002A, 0x4800},
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[0]
 {0x0F12, 0xFFB5}, //TVAR_wbt_pBaseCcms[1]
 {0x0F12, 0xFFE8}, //TVAR_wbt_pBaseCcms[2]
 {0x0F12, 0xFF20}, //TVAR_wbt_pBaseCcms[3]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[4]
 {0x0F12, 0xFF53}, //TVAR_wbt_pBaseCcms[5]
 {0x0F12, 0x0022}, //TVAR_wbt_pBaseCcms[6]
 {0x0F12, 0xFFEA}, //TVAR_wbt_pBaseCcms[7]
 {0x0F12, 0x01C2}, //TVAR_wbt_pBaseCcms[8]
 {0x0F12, 0x00C6}, //TVAR_wbt_pBaseCcms[9]
 {0x0F12, 0x0095}, //TVAR_wbt_pBaseCcms[10]
 {0x0F12, 0xFEFD}, //TVAR_wbt_pBaseCcms[11]
 {0x0F12, 0x0206}, //TVAR_wbt_pBaseCcms[12]
 {0x0F12, 0xFF7F}, //TVAR_wbt_pBaseCcms[13]
 {0x0F12, 0x0191}, //TVAR_wbt_pBaseCcms[14]
 {0x0F12, 0xFF06}, //TVAR_wbt_pBaseCcms[15]
 {0x0F12, 0x01BA}, //TVAR_wbt_pBaseCcms[16]
 {0x0F12, 0x0108}, //TVAR_wbt_pBaseCcms[17]

// INCA A
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[18]
 {0x0F12, 0xFFB5}, //TVAR_wbt_pBaseCcms[19]
 {0x0F12, 0xFFE8}, //TVAR_wbt_pBaseCcms[20]
 {0x0F12, 0xFF20}, //TVAR_wbt_pBaseCcms[21]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[22]
 {0x0F12, 0xFF53}, //TVAR_wbt_pBaseCcms[23]
 {0x0F12, 0x0022}, //TVAR_wbt_pBaseCcms[24]
 {0x0F12, 0xFFEA}, //TVAR_wbt_pBaseCcms[25]
 {0x0F12, 0x01C2}, //TVAR_wbt_pBaseCcms[26]
 {0x0F12, 0x00C6}, //TVAR_wbt_pBaseCcms[27]
 {0x0F12, 0x0095}, //TVAR_wbt_pBaseCcms[28]
 {0x0F12, 0xFEFD}, //TVAR_wbt_pBaseCcms[29]
 {0x0F12, 0x0206}, //TVAR_wbt_pBaseCcms[30]
 {0x0F12, 0xFF7F}, //TVAR_wbt_pBaseCcms[31]
 {0x0F12, 0x0191}, //TVAR_wbt_pBaseCcms[32]
 {0x0F12, 0xFF06}, //TVAR_wbt_pBaseCcms[33]
 {0x0F12, 0x01BA}, //TVAR_wbt_pBaseCcms[34]
 {0x0F12, 0x0108}, //TVAR_wbt_pBaseCcms[35]
//Warm White
 {0x0F12, 0x0208}, //TVAR_wbt_pBaseCcms[36]
 {0x0F12, 0xFFB5}, //TVAR_wbt_pBaseCcms[37]
 {0x0F12, 0xFFE8}, //TVAR_wbt_pBaseCcms[38]
 {0x0F12, 0xFF20}, //TVAR_wbt_pBaseCcms[39]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[40]
 {0x0F12, 0xFF53}, //TVAR_wbt_pBaseCcms[41]
 {0x0F12, 0x0022}, //TVAR_wbt_pBaseCcms[42]
 {0x0F12, 0xFFEA}, //TVAR_wbt_pBaseCcms[43]
 {0x0F12, 0x01C2}, //TVAR_wbt_pBaseCcms[44]
 {0x0F12, 0x00C6}, //TVAR_wbt_pBaseCcms[45]
 {0x0F12, 0x0095}, //TVAR_wbt_pBaseCcms[46]
 {0x0F12, 0xFEFD}, //TVAR_wbt_pBaseCcms[47]
 {0x0F12, 0x0206}, //TVAR_wbt_pBaseCcms[48]
 {0x0F12, 0xFF7F}, //TVAR_wbt_pBaseCcms[49]
 {0x0F12, 0x0191}, //TVAR_wbt_pBaseCcms[50]
 {0x0F12, 0xFF06}, //TVAR_wbt_pBaseCcms[51]
 {0x0F12, 0x01BA}, //TVAR_wbt_pBaseCcms[52]
 {0x0F12, 0x0108}, //TVAR_wbt_pBaseCcms[53]
//Cool White
 {0x0F12, 0x0204}, //TVAR_wbt_pBaseCcms[54]
 {0x0F12, 0xFFB2}, //TVAR_wbt_pBaseCcms[55]
 {0x0F12, 0xFFF5}, //TVAR_wbt_pBaseCcms[56]
 {0x0F12, 0xFEF1}, //TVAR_wbt_pBaseCcms[57]
 {0x0F12, 0x014E}, //TVAR_wbt_pBaseCcms[58]
 {0x0F12, 0xFF18}, //TVAR_wbt_pBaseCcms[59]
 {0x0F12, 0xFFE6}, //TVAR_wbt_pBaseCcms[60]
 {0x0F12, 0xFFDD}, //TVAR_wbt_pBaseCcms[61]
 {0x0F12, 0x01B2}, //TVAR_wbt_pBaseCcms[62]
 {0x0F12, 0x00F2}, //TVAR_wbt_pBaseCcms[63]
 {0x0F12, 0x00CA}, //TVAR_wbt_pBaseCcms[64]
 {0x0F12, 0xFF48}, //TVAR_wbt_pBaseCcms[65]
 {0x0F12, 0x0151}, //TVAR_wbt_pBaseCcms[66]
 {0x0F12, 0xFF50}, //TVAR_wbt_pBaseCcms[67]
 {0x0F12, 0x0147}, //TVAR_wbt_pBaseCcms[68]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[69]
 {0x0F12, 0x0187}, //TVAR_wbt_pBaseCcms[70]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[71]
//D50
 {0x0F12, 0x0204}, //TVAR_wbt_pBaseCcms[72]
 {0x0F12, 0xFFB2}, //TVAR_wbt_pBaseCcms[73]
 {0x0F12, 0xFFF5}, //TVAR_wbt_pBaseCcms[74]
 {0x0F12, 0xFEF1}, //TVAR_wbt_pBaseCcms[75]
 {0x0F12, 0x014E}, //TVAR_wbt_pBaseCcms[76]
 {0x0F12, 0xFF18}, //TVAR_wbt_pBaseCcms[77]
 {0x0F12, 0xFFE6}, //TVAR_wbt_pBaseCcms[78]
 {0x0F12, 0xFFDD}, //TVAR_wbt_pBaseCcms[79]
 {0x0F12, 0x01B2}, //TVAR_wbt_pBaseCcms[80]
 {0x0F12, 0x00F2}, //TVAR_wbt_pBaseCcms[81]
 {0x0F12, 0x00CA}, //TVAR_wbt_pBaseCcms[82]
 {0x0F12, 0xFF48}, //TVAR_wbt_pBaseCcms[83]
 {0x0F12, 0x0151}, //TVAR_wbt_pBaseCcms[84]
 {0x0F12, 0xFF50}, //TVAR_wbt_pBaseCcms[85]
 {0x0F12, 0x0147}, //TVAR_wbt_pBaseCcms[86]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[87]
 {0x0F12, 0x0187}, //TVAR_wbt_pBaseCcms[88]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[89]
//D65
 {0x0F12, 0x0204}, //TVAR_wbt_pBaseCcms[90]
 {0x0F12, 0xFFB2}, //TVAR_wbt_pBaseCcms[91]
 {0x0F12, 0xFFF5}, //TVAR_wbt_pBaseCcms[92]
 {0x0F12, 0xFEF1}, //TVAR_wbt_pBaseCcms[93]
 {0x0F12, 0x014E}, //TVAR_wbt_pBaseCcms[94]
 {0x0F12, 0xFF18}, //TVAR_wbt_pBaseCcms[95]
 {0x0F12, 0xFFE6}, //TVAR_wbt_pBaseCcms[96]
 {0x0F12, 0xFFDD}, //TVAR_wbt_pBaseCcms[97]
 {0x0F12, 0x01B2}, //TVAR_wbt_pBaseCcms[98]
 {0x0F12, 0x00F2}, //TVAR_wbt_pBaseCcms[99]
 {0x0F12, 0x00CA}, //TVAR_wbt_pBaseCcms[100]
 {0x0F12, 0xFF48}, //TVAR_wbt_pBaseCcms[101]
 {0x0F12, 0x0151}, //TVAR_wbt_pBaseCcms[102]
 {0x0F12, 0xFF50}, //TVAR_wbt_pBaseCcms[103]
 {0x0F12, 0x0147}, //TVAR_wbt_pBaseCcms[104]
 {0x0F12, 0xFF75}, //TVAR_wbt_pBaseCcms[105]
 {0x0F12, 0x0187}, //TVAR_wbt_pBaseCcms[106]
 {0x0F12, 0x01BF}, //TVAR_wbt_pBaseCcms[107]
//Outdoor
 {0x0F12, 0x01E5}, //TVAR_wbt_pOutdoorCcm[0]
 {0x0F12, 0xFFA4}, //TVAR_wbt_pOutdoorCcm[1]
 {0x0F12, 0xFFDC}, //TVAR_wbt_pOutdoorCcm[2]
 {0x0F12, 0xFE90}, //TVAR_wbt_pOutdoorCcm[3]
 {0x0F12, 0x013F}, //TVAR_wbt_pOutdoorCcm[4]
 {0x0F12, 0xFF1B}, //TVAR_wbt_pOutdoorCcm[5]
 {0x0F12, 0xFFD2}, //TVAR_wbt_pOutdoorCcm[6]
 {0x0F12, 0xFFDF}, //TVAR_wbt_pOutdoorCcm[7]
 {0x0F12, 0x0236}, //TVAR_wbt_pOutdoorCcm[8]
 {0x0F12, 0x00EC}, //TVAR_wbt_pOutdoorCcm[9]
 {0x0F12, 0x00F8}, //TVAR_wbt_pOutdoorCcm[10]
 {0x0F12, 0xFF34}, //TVAR_wbt_pOutdoorCcm[11]
 {0x0F12, 0x01CE}, //TVAR_wbt_pOutdoorCcm[12]
 {0x0F12, 0xFF83}, //TVAR_wbt_pOutdoorCcm[13]
 {0x0F12, 0x0195}, //TVAR_wbt_pOutdoorCcm[14]
 {0x0F12, 0xFEF3}, //TVAR_wbt_pOutdoorCcm[15]
 {0x0F12, 0x0126}, //TVAR_wbt_pOutdoorCcm[16]
 {0x0F12, 0x0162}, //TVAR_wbt_pOutdoorCcm[17]


	{0x002a, 0x48d8},
	{0x0f12, 0x01dc},
	{0x0f12, 0xff91},
	{0x0f12, 0xffe9},
	{0x0f12, 0xfeb5},
	{0x0f12, 0x013a},
	{0x0f12, 0xfeff},
	{0x0f12, 0xffb7},
	{0x0f12, 0xfff5},
	{0x0f12, 0x0237},
	{0x0f12, 0x00d5},
	{0x0f12, 0x0101},
	{0x0f12, 0xff39},
	{0x0f12, 0x01ce},
	{0x0f12, 0xff83},
	{0x0f12, 0x0195},
	{0x0f12, 0xfef3},
	{0x0f12, 0x014f},
	{0x0f12, 0x0137},
	//16.gamma
			{0x002A, 0x0734},
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[0][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[0][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[0][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[0][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[0][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[0][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[0][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[0][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[0][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[0][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[0][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[0][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[0][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[0][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[0][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[0][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[0][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[0][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[0][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[0][19]               
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[1][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[1][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[1][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[1][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[1][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[1][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[1][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[1][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[1][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[1][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[1][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[1][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[1][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[1][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[1][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[1][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[1][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[1][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[1][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[1][19]               
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[2][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[2][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[2][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[2][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[2][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[2][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[2][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[2][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[2][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[2][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[2][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[2][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[2][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[2][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[2][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[2][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[2][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[2][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[2][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[2][19] 


/*
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
		{0x0F12, 0x0000},//saRR_usDualGammaLutRGBIndoor[0][0]
		{0x0F12, 0x0004},//saRR_usDualGammaLutRGBIndoor[0][1]
		{0x0F12, 0x000B},//saRR_usDualGammaLutRGBIndoor[0][2]
		{0x0F12, 0x001B},//saRR_usDualGammaLutRGBIndoor[0][3]
		{0x0F12, 0x0046},//saRR_usDualGammaLutRGBIndoor[0][4]
		{0x0F12, 0x00AE},//saRR_usDualGammaLutRGBIndoor[0][5]
		{0x0F12, 0x011E},//saRR_usDualGammaLutRGBIndoor[0][6]
		{0x0F12, 0x0154},//saRR_usDualGammaLutRGBIndoor[0][7]
		{0x0F12, 0x0184},//saRR_usDualGammaLutRGBIndoor[0][8]
		{0x0F12, 0x01C6},//saRR_usDualGammaLutRGBIndoor[0][9]
		{0x0F12, 0x01F8},//saRR_usDualGammaLutRGBIndoor[0][10]
		{0x0F12, 0x0222},//saRR_usDualGammaLutRGBIndoor[0][11]
		{0x0F12, 0x0247},//saRR_usDualGammaLutRGBIndoor[0][12]
		{0x0F12, 0x0282},//saRR_usDualGammaLutRGBIndoor[0][13]
		{0x0F12, 0x02B5},//saRR_usDualGammaLutRGBIndoor[0][14]
		{0x0F12, 0x030F},//saRR_usDualGammaLutRGBIndoor[0][15]
		{0x0F12, 0x035F},//saRR_usDualGammaLutRGBIndoor[0][16]
		{0x0F12, 0x03A2},//saRR_usDualGammaLutRGBIndoor[0][17]
		{0x0F12, 0x03D8},//saRR_usDualGammaLutRGBIndoor[0][18]
		{0x0F12, 0x03FF},//saRR_usDualGammaLutRGBIndoor[0][19]
	*/	
	/*
	{0x002a, 0x0734},
	{0x0f12, 0x0000},   //0000  //saRR_usDualGammaLutRGBIndoor[0][0]        
	{0x0f12, 0x000A},   //000A  //saRR_usDualGammaLutRGBIndoor[0][1]        
	{0x0f12, 0x0016},   //0016  //saRR_usDualGammaLutRGBIndoor[0][2]        
	{0x0f12, 0x0030},   //0030  //saRR_usDualGammaLutRGBIndoor[0][3]        
	{0x0f12, 0x0066},   //0066  //saRR_usDualGammaLutRGBIndoor[0][4]        
	{0x0f12, 0x00C3},   //00D5  //saRR_usDualGammaLutRGBIndoor[0][5]        
	{0x0f12, 0x0112},   //0138  //saRR_usDualGammaLutRGBIndoor[0][6]        
	{0x0f12, 0x0136},   //0163  //saRR_usDualGammaLutRGBIndoor[0][7]        
	{0x0f12, 0x0152},   //0189  //saRR_usDualGammaLutRGBIndoor[0][8]        
	{0x0f12, 0x0186},   //01C6  //saRR_usDualGammaLutRGBIndoor[0][9]        
	{0x0f12, 0x01AE},   //01F8  //saRR_usDualGammaLutRGBIndoor[0][10]       
	{0x0f12, 0x01D6},   //0222  //saRR_usDualGammaLutRGBIndoor[0][11]       
	{0x0f12, 0x01FA},   //0247  //saRR_usDualGammaLutRGBIndoor[0][12]       
	{0x0f12, 0x024D},   //0282  //saRR_usDualGammaLutRGBIndoor[0][13]       
	{0x0f12, 0x0289},   //02B5  //saRR_usDualGammaLutRGBIndoor[0][14]       
	{0x0f12, 0x02E5},   //030F  //saRR_usDualGammaLutRGBIndoor[0][15]       
	{0x0f12, 0x0338},   //035F  //saRR_usDualGammaLutRGBIndoor[0][16]       
	{0x0f12, 0x038C},   //03A2  //saRR_usDualGammaLutRGBIndoor[0][17]       
	{0x0f12, 0x03D0},   //03D8  //saRR_usDualGammaLutRGBIndoor[0][18]       
	{0x0f12, 0x03FF},   //03FF  //saRR_usDualGammaLutRGBIndoor[0][19]       
	{0x0f12, 0x0000},   //0000  //saRR_usDualGammaLutRGBIndoor[1][0]        
	{0x0f12, 0x000A},   //000A  //saRR_usDualGammaLutRGBIndoor[1][1]        
	{0x0f12, 0x0016},   //0016  //saRR_usDualGammaLutRGBIndoor[1][2]        
	{0x0f12, 0x0030},   //0030  //saRR_usDualGammaLutRGBIndoor[1][3]        
	{0x0f12, 0x0066},   //0066  //saRR_usDualGammaLutRGBIndoor[1][4]        
	{0x0f12, 0x00D5},   //00D5  //saRR_usDualGammaLutRGBIndoor[1][5]        
	{0x0f12, 0x0138},   //0138  //saRR_usDualGammaLutRGBIndoor[1][6]        
	{0x0f12, 0x0163},   //0163  //saRR_usDualGammaLutRGBIndoor[1][7]        
	{0x0f12, 0x0189},   //0189  //saRR_usDualGammaLutRGBIndoor[1][8]        
	{0x0f12, 0x01C6},   //01C6  //saRR_usDualGammaLutRGBIndoor[1][9]        
	{0x0f12, 0x01F8},   //01F8  //saRR_usDualGammaLutRGBIndoor[1][10]       
	{0x0f12, 0x0222},   //0222  //saRR_usDualGammaLutRGBIndoor[1][11]       
	{0x0f12, 0x0247},   //0247  //saRR_usDualGammaLutRGBIndoor[1][12]       
	{0x0f12, 0x0282},   //0282  //saRR_usDualGammaLutRGBIndoor[1][13]       
	{0x0f12, 0x02B5},   //02B5  //saRR_usDualGammaLutRGBIndoor[1][14]       
	{0x0f12, 0x030F},   //030F  //saRR_usDualGammaLutRGBIndoor[1][15]       
	{0x0f12, 0x035F},   //035F  //saRR_usDualGammaLutRGBIndoor[1][16]       
	{0x0f12, 0x03A2},   //03A2  //saRR_usDualGammaLutRGBIndoor[1][17]       
	{0x0f12, 0x03D8},   //03D8  //saRR_usDualGammaLutRGBIndoor[1][18]       
	{0x0f12, 0x03FF},   //03FF  //saRR_usDualGammaLutRGBIndoor[1][19]       
	{0x0f12, 0x0000},   //0000  //saRR_usDualGammaLutRGBIndoor[2][0]        
	{0x0f12, 0x000A},   //000A  //saRR_usDualGammaLutRGBIndoor[2][1]        
	{0x0f12, 0x0016},   //0016  //saRR_usDualGammaLutRGBIndoor[2][2]        
	{0x0f12, 0x0030},   //0030  //saRR_usDualGammaLutRGBIndoor[2][3]        
	{0x0f12, 0x0066},   //0066  //saRR_usDualGammaLutRGBIndoor[2][4]        
	{0x0f12, 0x00D5},   //00D5  //saRR_usDualGammaLutRGBIndoor[2][5]        
	{0x0f12, 0x011E},   //0138  //saRR_usDualGammaLutRGBIndoor[2][6]        
	{0x0f12, 0x013A},   //0163  //saRR_usDualGammaLutRGBIndoor[2][7]        
	{0x0f12, 0x0156},   //0189  //saRR_usDualGammaLutRGBIndoor[2][8]        
	{0x0f12, 0x0182},   //01C6  //saRR_usDualGammaLutRGBIndoor[2][9]        
	{0x0f12, 0x01AE},   //01F8  //saRR_usDualGammaLutRGBIndoor[2][10]       
	{0x0f12, 0x01D6},   //0222  //saRR_usDualGammaLutRGBIndoor[2][11]       
	{0x0f12, 0x0202},   //0247  //saRR_usDualGammaLutRGBIndoor[2][12]       
	{0x0f12, 0x0251},   //0282  //saRR_usDualGammaLutRGBIndoor[2][13]       
	{0x0f12, 0x028D},   //02B5  //saRR_usDualGammaLutRGBIndoor[2][14]       
	{0x0f12, 0x02D9},   //030F  //saRR_usDualGammaLutRGBIndoor[2][15]       
	{0x0f12, 0x0340},   //035F  //saRR_usDualGammaLutRGBIndoor[2][16]       
	{0x0f12, 0x0388},   //03A2  //saRR_usDualGammaLutRGBIndoor[2][17]       
	{0x0f12, 0x03C8},   //03D8  //saRR_usDualGammaLutRGBIndoor[2][18]       
	{0x0f12, 0x03FF},   //03FF  //saRR_usDualGammaLutRGBIndoor[2][19]   
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[0][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[0][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[0][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[0][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[0][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[0][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[0][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[0][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[0][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[0][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[0][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[0][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[0][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[0][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[0][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[0][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[0][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[0][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[0][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[0][19]               
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[1][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[1][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[1][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[1][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[1][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[1][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[1][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[1][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[1][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[1][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[1][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[1][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[1][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[1][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[1][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[1][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[1][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[1][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[1][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[1][19]               
	{0x0f12, 0x0000},  //saRR_usDualGammaLutRGBOutdoor[2][0]                
	{0x0f12, 0x000b},  //saRR_usDualGammaLutRGBOutdoor[2][1]                
	{0x0f12, 0x0019},  //saRR_usDualGammaLutRGBOutdoor[2][2]                
	{0x0f12, 0x0036},  //saRR_usDualGammaLutRGBOutdoor[2][3]                
	{0x0f12, 0x006f},  //saRR_usDualGammaLutRGBOutdoor[2][4]                
	{0x0f12, 0x00d8},  //saRR_usDualGammaLutRGBOutdoor[2][5]                
	{0x0f12, 0x0135},  //saRR_usDualGammaLutRGBOutdoor[2][6]                
	{0x0f12, 0x015f},  //saRR_usDualGammaLutRGBOutdoor[2][7]                
	{0x0f12, 0x0185},  //saRR_usDualGammaLutRGBOutdoor[2][8]                
	{0x0f12, 0x01c1},  //saRR_usDualGammaLutRGBOutdoor[2][9]                
	{0x0f12, 0x01f3},  //saRR_usDualGammaLutRGBOutdoor[2][10]               
	{0x0f12, 0x0220},  //saRR_usDualGammaLutRGBOutdoor[2][11]               
	{0x0f12, 0x024a},  //saRR_usDualGammaLutRGBOutdoor[2][12]               
	{0x0f12, 0x0291},  //saRR_usDualGammaLutRGBOutdoor[2][13]               
	{0x0f12, 0x02d0},  //saRR_usDualGammaLutRGBOutdoor[2][14]               
	{0x0f12, 0x032a},  //saRR_usDualGammaLutRGBOutdoor[2][15]               
	{0x0f12, 0x036a},  //saRR_usDualGammaLutRGBOutdoor[2][16]               
	{0x0f12, 0x039f},  //saRR_usDualGammaLutRGBOutdoor[2][17]               
	{0x0f12, 0x03cc},  //saRR_usDualGammaLutRGBOutdoor[2][18]               
	{0x0f12, 0x03f9},  //saRR_usDualGammaLutRGBOutdoor[2][19]  
	*/
	{0x002A, 0x0230},
	{0x0f12, 0x0000},
	{0x0f12, 0x0020},
	{0x0f12, 0x0018},
	{0x0f12, 0x0000},
	{0x002A, 0x023A},
	{0x0f12, 0x0120},

//================================================================================== 
// 17.AFIT                                                                           
//==================================================================================            
 {0x002a, 0x0944},
  {0x0f12, 0x0050}, //afit_uNoiseIndInDoor 0x0050                                                                              
	{0x0f12, 0x00b0}, //afit_uNoiseIndInDoor 0x00b0                                                                              
	{0x0f12, 0x0196}, //afit_uNoiseIndInDoor    0x0196                                                                           
	{0x0f12, 0x0245}, //afit_uNoiseIndInDoor                                                                              
	{0x0f12, 0x0300}, //afit_uNoiseIndInDoor                                                                              
	{0x002a, 0x0938},                                                                                                     
	{0x0f12, 0x0000}, // on/off AFIT by NB option                                                                         
	{0x0f12, 0x0014}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x00d2}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x0384}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x07d0}, //SARR_uNormBrInDoor                                                                                
	{0x0f12, 0x1388}, //SARR_uNormBrInDoor                                                                                
	{0x002a, 0x0976},                                                                                                     
	{0x0f12, 0x0070}, //afit_usGamutTh                                                                                    
	{0x0f12, 0x0005}, //afit_usNeargrayOffset                                                                             
	{0x0f12, 0x0000}, //afit_bUseSenBpr                                                                                   
	{0x0f12, 0x01cc}, //afit_usBprThr_0_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_1_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_2_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_3_                                                                                  
	{0x0f12, 0x01cc}, //afit_usBprThr_4_                                                                                  
	{0x0f12, 0x0180}, //afit_NIContrastAFITValue                                                                          
	{0x0f12, 0x0196}, //afit_NIContrastTh                                                                                 
	{0x002a, 0x098c},                                                                                                     
	{0x0f12, 0x0000}, //7000098C//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0x0000}, //7000098E//AFIT16_CONTRAST                                                                         
	{0x0f12, 0xfff0}, //70000990//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000992//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000994//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00c0}, //70000996//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000998//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //7000099A//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x005f}, //7000099C//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01f4}, //7000099E//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //700009A0//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //700009A2//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00a0}, //700009A4//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //700009A6//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //700009A8//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0040}, //700009AA//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x00a0}, //700009AC//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //700009AE//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //700009B0//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //700009B2//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x3604}, //700009B4//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x032a}, //700009B6//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0403}, //700009B8//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x1b06}, //700009BA//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x6015}, //700009BC//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x00c0}, //700009BE//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x6080}, //700009C0//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x4080}, //700009C2//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0640}, //700009C4//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //700009C6//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //700009C8//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xff01}, //700009CA//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0000}, //700009CC//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0400}, //700009CE//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x365a}, //700009D0//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x102a}, //700009D2//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000b}, //700009D4//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x0600}, //700009D6//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5a0f}, //700009D8//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0505}, //700009DA//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //700009DC//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //700009DE//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //700009E0//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x3028}, //700009E2//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x0468}, //700009E4//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh 0x0418                                           
	{0x0f12, 0x0101}, //700009E6//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0x0800}, //700009E8//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x1804}, //700009EA//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //700009EC//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //700009EE//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //700009F0//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //700009F2//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //700009F4//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //700009F6//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //700009F8//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1e10}, //700009FA//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000b}, //700009FC//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //700009FE//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000A00//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000A02//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000A04//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000A06//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000A08//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000A0A//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000A0C//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000A0E//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x1c04}, //70000A10//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0214}, //70000A12//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1002}, //70000A14//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0610}, //70000A16//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1a02}, //70000A18//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x4a18}, //70000A1A//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000A1C//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0348}, //70000A1E//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000A20//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0a0a}, //70000A22//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000A24//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x0505}, //70000A26//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh                                
	{0x0f12, 0xc080}, //70000A28//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x2a36}, //70000A2A//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xffff}, //70000A2C//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000A2E//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0a01}, //70000A30//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010a}, //70000A32//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x3601}, //70000A34//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x242a}, //70000A36//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x3660}, //70000A38//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xff2a}, //70000A3A//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08ff}, //70000A3C//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000A3E//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000A40//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000A42//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0x0000}, //70000A44//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000A46//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000A48//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000A4A//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00c0}, //70000A4C//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000A4E//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000A50//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0051}, //70000A52//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01f4}, //70000A54//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000A56//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000A58//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00a0}, //70000A5A//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000A5C//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000A5E//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000A60//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000A62//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000A64//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000A66//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000A68//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x2404}, //70000A6A//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x031b}, //70000A6C//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0103}, //70000A6E//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x1205}, //70000A70//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x400d}, //70000A72//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0080}, //70000A74//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x2080}, //70000A76//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x3040}, //70000A78//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0630}, //70000A7A//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000A7C//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000A7E//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xff01}, //70000A80//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000A82//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0300}, //70000A84//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x245a}, //70000A86//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1018}, //70000A88//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000b}, //70000A8A//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x0b00}, //70000A8C//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5a0f}, //70000A8E//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0505}, //70000A90//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000A92//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000A94//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000A96//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x3428}, //70000A98//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x046c}, //70000A9A//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh   0x041c                                         
	{0x0f12, 0x0101}, //70000A9C//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0x0800}, //70000A9E//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x1004}, //70000AA0//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000AA2//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000AA4//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000AA6//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000AA8//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000AAA//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000AAC//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000AAE//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1e10}, //70000AB0//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000b}, //70000AB2//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000AB4//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000AB6//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000AB8//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000ABA//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000ABC//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000ABE//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000AC0//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000AC2//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000AC4//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x1f04}, //70000AC6//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0218}, //70000AC8//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1102}, //70000ACA//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0611}, //70000ACC//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1a02}, //70000ACE//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x8018}, //70000AD0//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000AD2//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0380}, //70000AD4//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000AD6//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0a0a}, //70000AD8//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000ADA//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x0505}, //70000ADC//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh 0x1b24                               
	{0x0f12, 0xc080}, //70000ADE//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x1d22}, //70000AE0//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xffff}, //70000AE2//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000AE4//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0a01}, //70000AE6//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010a}, //70000AE8//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x2401}, //70000AEA//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x241b}, //70000AEC//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x1e60}, //70000AEE//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xff18}, //70000AF0//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08ff}, //70000AF2//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000AF4//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000AF6//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000AF8//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0x0000}, //70000AFA//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000AFC//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000AFE//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000B00//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00c0}, //70000B02//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000B04//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000B06//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0043}, //70000B08//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01f4}, //70000B0A//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000B0C//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000B0E//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00a0}, //70000B10//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000B12//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000B14//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000B16//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000B18//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000B1A//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000B1C//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000B1E//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x1b04}, //70000B20//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x0312}, //70000B22//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0003}, //70000B24//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x0c03}, //70000B26//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x2806}, //70000B28//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0060}, //70000B2A//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x1580}, //70000B2C//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x2020}, //70000B2E//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0620}, //70000B30//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000B32//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000B34//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xff01}, //70000B36//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000B38//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0300}, //70000B3A//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x145a}, //70000B3C//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1010}, //70000B3E//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000b}, //70000B40//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x0e00}, //70000B42//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5a0f}, //70000B44//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0504}, //70000B46//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000B48//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000B4A//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000B4C//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x3828}, //70000B4E//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x0478}, //70000B50//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh  0x0428                                          
	{0x0f12, 0x0101}, //70000B52//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0x8000}, //70000B54//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x0a04}, //70000B56//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000B58//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000B5A//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000B5C//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000B5E//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000B60//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000B62//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000B64//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1e10}, //70000B66//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000b}, //70000B68//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000B6A//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000B6C//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000B6E//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000B70//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0207}, //70000B72//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000B74//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000B76//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000B78//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000B7A//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x2404}, //70000B7C//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0221}, //70000B7E//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1202}, //70000B80//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0613}, //70000B82//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1a02}, //70000B84//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x8018}, //70000B86//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000B88//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000B8A//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000B8C//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0a0a}, //70000B8E//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000B90//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x0404}, //70000B92//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh  0x141d                              
	{0x0f12, 0xc090}, //70000B94//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x0c0c}, //70000B96//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xffff}, //70000B98//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000B9A//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0a01}, //70000B9C//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010a}, //70000B9E//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x1b01}, //70000BA0//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x2412}, //70000BA2//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x0c60}, //70000BA4//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xff0c}, //70000BA6//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08ff}, //70000BA8//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000BAA//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000BAC//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000BAE//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0x0000}, //70000BB0//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000BB2//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000BB4//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000BB6//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00c0}, //70000BB8//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000BBA//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000BBC//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //70000BBE//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01f4}, //70000BC0//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000BC2//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000BC4//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00a0}, //70000BC6//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000BC8//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000BCA//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000BCC//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000BCE//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000BD0//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000BD2//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000BD4//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x1504}, //70000BD6//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x030f}, //70000BD8//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0003}, //70000BDA//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x0902}, //70000BDC//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x2004}, //70000BDE//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0050}, //70000BE0//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x1140}, //70000BE2//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x201c}, //70000BE4//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0620}, //70000BE6//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000BE8//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000BEA//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xff01}, //70000BEC//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000BEE//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0300}, //70000BF0//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x145a}, //70000BF2//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1010}, //70000BF4//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000b}, //70000BF6//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x1000}, //70000BF8//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5a0f}, //70000BFA//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0503}, //70000BFC//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000BFE//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000C00//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000C02//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x3c28}, //70000C04//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x047c}, //70000C06//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh  0x042c                                          
	{0x0f12, 0x0101}, //70000C08//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0xff00}, //70000C0A//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x0904}, //70000C0C//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000C0E//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000C10//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000C12//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000C14//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000C16//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000C18//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000C1A//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1e10}, //70000C1C//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000b}, //70000C1E//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000C20//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000C22//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000C24//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000C26//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0206}, //70000C28//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000C2A//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000C2C//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0305}, //70000C2E//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0406}, //70000C30//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x2804}, //70000C32//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x0228}, //70000C34//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1402}, //70000C36//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0618}, //70000C38//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1a02}, //70000C3A//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x8018}, //70000C3C//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000C3E//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000C40//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000C42//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0a0a}, //70000C44//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000C46//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x0404}, //70000C48//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh 0x1117                               
	{0x0f12, 0xE0a0}, //70000C4A//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x0a0a}, //70000C4C//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xffff}, //70000C4E//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000C50//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0a01}, //70000C52//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010a}, //70000C54//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x1501}, //70000C56//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x240f}, //70000C58//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x0a60}, //70000C5A//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xff0a}, //70000C5C//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08ff}, //70000C5E//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000C60//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001}, //70000C62//AFITB_bnr_nClustLevel_C    [0]                                                          
	{0x0f12, 0x0000}, //70000C64//AFIT16_BRIGHTNESS                                                                       
	{0x0f12, 0x0000}, //70000C66//AFIT16_CONTRAST                                                                         
	{0x0f12, 0x0000}, //70000C68//AFIT16_SATURATION                                                                       
	{0x0f12, 0x0000}, //70000C6A//AFIT16_SHARP_BLUR                                                                       
	{0x0f12, 0x0000}, //70000C6C//AFIT16_GLAMOUR                                                                          
	{0x0f12, 0x00c0}, //70000C6E//AFIT16_bnr_edge_high                                                                    
	{0x0f12, 0x0064}, //70000C70//AFIT16_postdmsc_iLowBright                                                              
	{0x0f12, 0x0384}, //70000C72//AFIT16_postdmsc_iHighBright                                                             
	{0x0f12, 0x0032}, //70000C74//AFIT16_postdmsc_iLowSat                                                                 
	{0x0f12, 0x01f4}, //70000C76//AFIT16_postdmsc_iHighSat                                                                
	{0x0f12, 0x0070}, //70000C78//AFIT16_postdmsc_iTune                                                                   
	{0x0f12, 0x0040}, //70000C7A//AFIT16_yuvemix_mNegRanges_0                                                             
	{0x0f12, 0x00a0}, //70000C7C//AFIT16_yuvemix_mNegRanges_1                                                             
	{0x0f12, 0x0100}, //70000C7E//AFIT16_yuvemix_mNegRanges_2                                                             
	{0x0f12, 0x0010}, //70000C80//AFIT16_yuvemix_mPosRanges_0                                                             
	{0x0f12, 0x0060}, //70000C82//AFIT16_yuvemix_mPosRanges_1                                                             
	{0x0f12, 0x0100}, //70000C84//AFIT16_yuvemix_mPosRanges_2                                                             
	{0x0f12, 0x1430}, //70000C86//AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh                                         
	{0x0f12, 0x0201}, //70000C88//AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh                                    
	{0x0f12, 0x0204}, //70000C8A//AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh                               
	{0x0f12, 0x0f04}, //70000C8C//AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low                                   
	{0x0f12, 0x030c}, //70000C8E//AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low                                  
	{0x0f12, 0x0003}, //70000C90//AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin                               
	{0x0f12, 0x0602}, //70000C92//AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow                                 
	{0x0f12, 0x1803}, //70000C94//AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH                            
	{0x0f12, 0x0040}, //70000C96//AFIT8_bnr_iNormalizedSTD_Limit [7:0] AFIT8_bnr_iDirNRTune                               
	{0x0f12, 0x0e20}, //70000C98//AFIT8_bnr_iDirMinThres [7:0] AFIT8_bnr_iDirFltDiffThresHigh                             
	{0x0f12, 0x2018}, //70000C9A//AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh                     
	{0x0f12, 0x0620}, //70000C9C//AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed                      
	{0x0f12, 0x0306}, //70000C9E//AFIT8_bnr_iHighMaxSlopeAllowed [7:0] AFIT8_bnr_iLowSlopeThresh                          
	{0x0f12, 0x2003}, //70000CA0//AFIT8_bnr_iHighSlopeThresh [7:0] AFIT8_bnr_iSlopenessTH                                 
	{0x0f12, 0xff01}, //70000CA2//AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit                          
	{0x0f12, 0x0404}, //70000CA4//AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2                               
	{0x0f12, 0x0200}, //70000CA6//AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower                                    
	{0x0f12, 0x145a}, //70000CA8//AFIT8_bnr_iRadialLimit [7:0] AFIT8_ee_iFSMagThLow                                       
	{0x0f12, 0x1010}, //70000CAA//AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow                                      
	{0x0f12, 0x000b}, //70000CAC//AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow                                         
	{0x0f12, 0x1200}, //70000CAE//AFIT8_ee_iFSThHigh [7:0] AFIT8_ee_iFSmagPower                                           
	{0x0f12, 0x5a0f}, //70000CB0//AFIT8_ee_iFSVarCountTh [7:0] AFIT8_ee_iRadialLimit                                      
	{0x0f12, 0x0502}, //70000CB2//AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope                                 
	{0x0f12, 0x1802}, //70000CB4//AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR                                         
	{0x0f12, 0x0000}, //70000CB6//AFIT8_ee_iROADSubMaxNR [7:0] AFIT8_ee_iROADSubThres                                     
	{0x0f12, 0x2006}, //70000CB8//AFIT8_ee_iROADNeiThres [7:0] AFIT8_ee_iROADNeiMaxNR                                     
	{0x0f12, 0x4028}, //70000CBA//AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen                                    
	{0x0f12, 0x0480}, //70000CBC//AFIT8_ee_iWSharpen [7:0] AFIT8_ee_iMShThresh 0x0430                                            
	{0x0f12, 0x0101}, //70000CBE//AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative                                    
	{0x0f12, 0xff00}, //70000CC0//AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle                                   
	{0x0f12, 0x0804}, //70000CC2//AFIT8_ee_iReduceEdgeThresh [7:0] AFIT8_dmsc_iEnhThresh                                  
	{0x0f12, 0x4008}, //70000CC4//AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh                                 
	{0x0f12, 0x0540}, //70000CC6//AFIT8_dmsc_iDemBlurLow [7:0] AFIT8_dmsc_iDemBlurRange                                   
	{0x0f12, 0x8006}, //70000CC8//AFIT8_dmsc_iDecisionThresh [7:0] AFIT8_dmsc_iCentGrad                                   
	{0x0f12, 0x0020}, //70000CCA//AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal                                  
	{0x0f12, 0x0000}, //70000CCC//AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh                           
	{0x0f12, 0x1800}, //70000CCE//AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat                               
	{0x0f12, 0x0000}, //70000CD0//AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit                            
	{0x0f12, 0x1e10}, //70000CD2//AFIT8_postdmsc_iBCoeff [7:0] AFIT8_postdmsc_iGCoeff                                     
	{0x0f12, 0x000b}, //70000CD4//AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0                             
	{0x0f12, 0x0607}, //70000CD6//AFIT8_yuvemix_mNegSlopes_1 [7:0] AFIT8_yuvemix_mNegSlopes_2                             
	{0x0f12, 0x0005}, //70000CD8//AFIT8_yuvemix_mNegSlopes_3 [7:0] AFIT8_yuvemix_mPosSlopes_0                             
	{0x0f12, 0x0607}, //70000CDA//AFIT8_yuvemix_mPosSlopes_1 [7:0] AFIT8_yuvemix_mPosSlopes_2                             
	{0x0f12, 0x0405}, //70000CDC//AFIT8_yuvemix_mPosSlopes_3 [7:0] AFIT8_yuviirnr_iXSupportY                              
	{0x0f12, 0x0205}, //70000CDE//AFIT8_yuviirnr_iXSupportUV [7:0] AFIT8_yuviirnr_iLowYNorm                               
	{0x0f12, 0x0304}, //70000CE0//AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm                             
	{0x0f12, 0x0409}, //70000CE2//AFIT8_yuviirnr_iHighUVNorm [7:0] AFIT8_yuviirnr_iYNormShift                             
	{0x0f12, 0x0306}, //70000CE4//AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y                        
	{0x0f12, 0x0407}, //70000CE6//AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y                     
	{0x0f12, 0x2c04}, //70000CE8//AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV                    
	{0x0f12, 0x022c}, //70000CEA//AFIT8_yuviirnr_iDiffThreshH_UV [7:0] AFIT8_yuviirnr_iMaxThreshL_Y                       
	{0x0f12, 0x1402}, //70000CEC//AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV                      
	{0x0f12, 0x0618}, //70000CEE//AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL                      
	{0x0f12, 0x1a02}, //70000CF0//AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL                      
	{0x0f12, 0x8018}, //70000CF2//AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower                      
	{0x0f12, 0x0080}, //70000CF4//AFIT8_RGBGamma2_iLinearity [7:0] AFIT8_RGBGamma2_iDarkReduce                            
	{0x0f12, 0x0080}, //70000CF6//AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset                              
	{0x0f12, 0x0180}, //70000CF8//AFIT8_RGB2YUV_iRGBGain [7:0] AFIT8_bnr_nClustLevel_H                                    
	{0x0f12, 0x0a0a}, //70000CFA//AFIT8_bnr_iClustMulT_H [7:0] AFIT8_bnr_iClustMulT_C                                     
	{0x0f12, 0x0101}, //70000CFC//AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C                               
	{0x0f12, 0x0404}, //70000CFE//AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh 0x0c0f                               
	{0x0f12, 0xE0a0}, //70000D00//AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower                                
	{0x0f12, 0x0808}, //70000D02//AFIT8_ee_iLowShDenoise [7:0] AFIT8_ee_iHighShDenoise                                    
	{0x0f12, 0xffff}, //70000D04//AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp                                
	{0x0f12, 0x0808}, //70000D06//AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope                           
	{0x0f12, 0x0a01}, //70000D08//AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin                          
	{0x0f12, 0x010a}, //70000D0A//AFIT8_bnr_iClustMulT_C_Bin [7:0] AFIT8_bnr_iClustThresh_H_Bin                           
	{0x0f12, 0x0f01}, //70000D0C//AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin                        
	{0x0f12, 0x240c}, //70000D0E//AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin                        
	{0x0f12, 0x0860}, //70000D10//AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin                         
	{0x0f12, 0xff08}, //70000D12//AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin                         
	{0x0f12, 0x08ff}, //70000D14//AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin                    
	{0x0f12, 0x0008}, //70000D16//AFIT8_ee_iReduceEdgeSlope_Bin [7:0]                                                     
	{0x0f12, 0x0001},   //70000D18 AFITB_bnr_nClustLevel_C    [0]   bWideWide[1]                                          
	{0x0f12, 0x23ce}, //70000D19//ConstAfitBaseVals                                                                       
	{0x0f12, 0xfdc8}, //70000D1A//ConstAfitBaseVals                                                                       
	{0x0f12, 0x112e}, //70000D1B//ConstAfitBaseVals                                                                       
	{0x0f12, 0x93a5}, //70000D1C//ConstAfitBaseVals                                                                       
	{0x0f12, 0xfe67}, //70000D1D//ConstAfitBaseVals                                                                       
	{0x0f12, 0x0000}, //70000D1E//ConstAfitBaseVals   
	                                                                    
	{0x002a, 0x0250},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x002a, 0x0494},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x002a, 0x0262},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0x002a, 0x02a6},
	{0x0f12, 0x0320},//#REG_0TC_PCFG_usWidth//Hsize:800//        //PCFG                                   
	{0x0f12, 0x0258},//#REG_0TC_PCFG_usHeight//Vsize:600//       //output image width:800                 
	{0x0f12, 0x0005},//#REG_0TC_PCFG_Format//5:YUV7:Raw9:JPG//   //output image height:600                
	{0x0f12, 0x4f1a},//#REG_0TC_PCFG_usMaxOut4KHzRate//          //yuv                                    
	{0x0f12, 0x4f1a},//#REG_0TC_PCFG_usMinOut4KHzRate//                                                   
	{0x0f12, 0x0100},//#REG_0TC_PCFG_OutClkPerPix88//                                                     
	{0x0f12, 0x0300},//#REG_0TC_PCFG_uBpp88//                                                             
	{0x0f12, 0x0052},//#REG_0TC_PCFG_PVIMask//                                                            
	{0x0f12, 0x0000},//#REG_0TC_PCFG_OIFMask//                                                            
	{0x0f12, 0x01e0},//#REG_0TC_PCFG_usJpegPacketSize//                                                   
	{0x0f12, 0x0000},//#REG_0TC_PCFG_usJpegTotalPackets//                                                 
	{0x0f12, 0x0000},//#REG_0TC_PCFG_uClockInd//                                                          
	{0x0f12, 0x0000},//#REG_0TC_PCFG_usFrTimeType//                                                       
	{0x0f12, 0x0001},//#REG_0TC_PCFG_FrRateQualityType//    0x0001   0x0002                                           
	{0x0f12, 0x0535},//29A//#REG_0TC_PCFG_usMaxFrTimeMsecMult10//                                         
	{0x0f12, 0x014d},//14D//#REG_0TC_PCFG_usMinFrTimeMsecMult10//                                         
	{0x002a, 0x02d0},
	{0x0f12, 0x0003},
	{0x0f12, 0x0003},
	{0x0f12, 0x0000},
	
	{0x002a, 0x0396},//CGFG
	{0x0f12, 0x0001},//#REG_0TC_CCFG_uCaptureMode//         
	{0x0f12, 0x0a00},//#REG_0TC_CCFG_usWidth//              //
	{0x0f12, 0x0780},//#REG_0TC_CCFG_usHeight//             //
	{0x0f12, 0x0005},//#REG_0TC_CCFG_Format//               //
	{0x0f12, 0x4f1a},//#REG_0TC_CCFG_usMaxOut4KHzRate//     
	{0x0f12, 0x4f1a},//#REG_0TC_CCFG_usMinOut4KHzRate//     
	{0x0f12, 0x0100},//#REG_0TC_CCFG_OutClkPerPix88//       
	{0x0f12, 0x0300},//#REG_0TC_CCFG_uBpp88//               
	{0x0f12, 0x0050},//#REG_0TC_CCFG_PVIMask//              
	{0x0f12, 0x0000},//#REG_0TC_CCFG_OIFMask//              
	{0x0f12, 0x01e0},//#REG_0TC_CCFG_usJpegPacketSize//     
	{0x0f12, 0x0000},//#REG_0TC_CCFG_usJpegTotalPackets//   
	{0x0f12, 0x0000},//#REG_0TC_CCFG_uClockInd//            
	{0x0f12, 0x0000},//#REG_0TC_CCFG_usFrTimeType//         
	{0x0f12, 0x0002},//#REG_0TC_CCFG_FrRateQualityType//    
	{0x0f12, 0x0535},//#REG_0TC_CCFG_usMaxFrTimeMsecMult10//
	{0x0f12, 0x0535},//#REG_0TC_CCFG_usMinFrTimeMsecMult10//
	
	{0x002a, 0x022c},
	{0x0f12, 0x0001},
	{0x0028, 0x7000},
	{0x002a, 0x0266},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0268},
	{0x0f12, 0x0001},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0270},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x023e},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0x0028, 0x7000},
	{0x002a, 0x01a8},
	{0x0f12, 0xaaaa},
	{0x0028, 0x147c},
	{0x0f12, 0x0180},
	{0x0028, 0x1482},
	{0x0f12, 0x0180},
	/* set 422 uyvy */
//	{0xffff, 0x0000},
//	{0xfcfc, 0xd000},
//	{0x0028, 0x7000},
	{0x002a, 0x02b4},
	{0x0f12, 0x0050},
	{0x002a, 0x03a6},
	{0x0f12, 0x0050},
	SensorEnd
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	{0x002a, 0x0396},//CCFG
	{0x0f12, 0x0001},//capture mode
	{0x0f12, 0x0a00},//2590
	{0x0f12, 0x0780},//1920
//	{0x0f12, 0x0a00},//2590
//	{0x0f12, 0x0780},//1920
	{0x0f12, 0x0005},//yuv
	{0x002a, 0x03ae},
	{0x0f12, 0x0000},
	{0x002a, 0x03b4},//
	{0x0f12, 0x0535},//framerate:5   0x7d0
	{0x0f12, 0x0535},    // 0x7d0
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	//set pvi mode
	{0x002a, 0x0216},//0216
	{0x0f12, 0x0000},    //0x0000
	/* set 422 uyvy */
//	{0xffff, 0x0000},
//	{0xfcfc, 0xd000},
//	{0x0028, 0x7000},
	{0x002a, 0x02b4},	//REG_0TC_PCFG_PVIMask
	{0x0f12, 0x0052},
	
	{0x002a, 0x03a6},   //REG_0TC_CCFG_PVIMask
	{0x0f12, 0x0052},
	/*//for AF
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x028c},
	{0x0f12, 0x0005},
	*/
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x002a, 0x0242},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x0244},
	{0x0f12, 0x0001},
	{0xffff, 0x00f4},//delay
	SensorEnd
};
/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	{0x002a, 0x0396},
	{0x0f12, 0x0001},
	{0x0f12, 0x0a20},
	{0x0f12, 0x0798},
	{0x0f12, 0x0005},//yuv
	{0x002a, 0x03ae},
	{0x0f12, 0x0000},
	{0x002a, 0x03b4},
	{0x0f12, 0x0535},//framerate:7.5
	{0x0f12, 0x0535},
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	//set pvi mode
	{0x002a, 0x0216},
	{0x0f12, 0x0000},
	/* set 422 uyvy */
//	{0xffff, 0x0000},
//	{0xfcfc, 0xd000},
//	{0x0028, 0x7000},
	{0x002a, 0x02b4},//REG_0TC_PCFG_PVIMask
	{0x0f12, 0x0052},
	{0x002a, 0x03a6},//REG_0TC_CCFG_PVIMask
	{0x0f12, 0x0052},
//for AF	
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x028c},
	{0x0f12, 0x0005},
	*/
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x002a, 0x0242},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x0244},
	{0x0f12, 0x0001},
	{0xffff, 0x00f4},//delay
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
	{0x002a, 0x18ac},
	{0x0f12, 0x0060},
	{0x0f12, 0x0060},
	{0x0f12, 0x05c0},
	{0x0f12, 0x05c0},
	{0x002a, 0x0250},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x002a, 0x0494},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x002a, 0x0262},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x02a6},//PCFG
	{0x0f12, 0x0320},//width:800
	{0x0f12, 0x0258},//height:600
	{0x0f12, 0x0005},//5:yuv
	{0x002a, 0x02bc},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x0f12, 0x0320},  //0x0535
	{0x0f12, 0x014D},
	
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x028c},
	{0x0f12, 0x0005},
	
	{0x0028, 0x7000},
	{0x002a, 0x0266},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0268},
	{0x0f12, 0x0001},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0270},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x023e},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	//framerate setting
	{0x002a, 0x03b4},
	{0x0f12, 0x0535},//framerate:15 //0x29a
	{0x0f12, 0x029a},
	//set pvi mode
	{0x002a, 0x0216},
	{0x0f12, 0x0000},
	{0x002a, 0x02b4},//REG_0TC_PCFG_PVIMask
	{0x0f12, 0x0052},
	{0x002a, 0x03a6},//REG_0TC_CCFG_PVIMask
	{0x0f12, 0x0052},
	
	SensorEnd
};
static struct rk_sensor_reg sensor_VGA_data[] =
{
	{0x002a, 0x18ac},
	{0x0f12, 0x0060},
	{0x0f12, 0x0060},
	{0x0f12, 0x05c0},
	{0x0f12, 0x05c0},
	{0x002a, 0x0250},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x002a, 0x0494},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x002a, 0x0262},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x02a6},//PCFG
	{0x0f12, 0x0280},//width:640
	{0x0f12, 0x01e0},//height:480
	{0x0f12, 0x0005},//5:yuv
	{0x002a, 0x02bc},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x0f12, 0x0320},  //0x0535
	{0x0f12, 0x014D},
	
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x028c},
	{0x0f12, 0x0005},
	
	{0x0028, 0x7000},
	{0x002a, 0x0266},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0268},
	{0x0f12, 0x0001},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0270},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x023e},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	//framerate setting
	{0x002a, 0x03b4},
	{0x0f12, 0x0535},//framerate:15 //0x29a
	{0x0f12, 0x029a},
	//set pvi mode
	{0x002a, 0x0216},
	{0x0f12, 0x0000},
	{0x002a, 0x02b4},//REG_0TC_PCFG_PVIMask
	{0x0f12, 0x0052},
	{0x002a, 0x03a6},//REG_0TC_CCFG_PVIMask
	{0x0f12, 0x0052},
	
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	{0x002a, 0x18ac},
	{0x0f12, 0x0060},
	{0x0f12, 0x0060},
	{0x0f12, 0x05c0},
	{0x0f12, 0x0a96},
	{0x002a, 0x0250},
	{0x0f12, 0x0a00},
	{0x0f12, 0x05a0},
	{0x0f12, 0x0010},
	{0x0f12, 0x00fc},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x002a, 0x0494},
	{0x0f12, 0x0a00},
	{0x0f12, 0x05a0},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x002a, 0x0262},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x02a6},//PCFG
	{0x0f12, 0x0500},//width
	{0x0f12, 0x02d0},//height
	{0x002a, 0x02bc},
	{0x0f12, 0x0000},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0x0f12, 0x014d},
	{0x0f12, 0x014d},
	{0x002a, 0x022c},
	{0x0f12, 0x0001},
	
		//for AF
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x028c},
	{0x0f12, 0x0005},
	
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x0266},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0268},
	{0x0f12, 0x0001},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0270},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x023e},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0xffff, 0x002c},//delay

	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
/*
	{0x002a, 0x18ac},
	{0x0f12, 0x0060},
	{0x0f12, 0x0060},
	{0x0f12, 0x05c0},
	{0x0f12, 0x0a96},
	{0x002a, 0x0250},
	{0x0f12, 0x0780},
	{0x0f12, 0x0438},
	{0x0f12, 0x014e},
	{0x0f12, 0x01b0},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0010},
	{0x0f12, 0x000c},
	{0x002a, 0x0494},
	{0x0f12, 0x0780},
	{0x0f12, 0x0438},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x0f12, 0x0a00},
	{0x0f12, 0x0780},
	{0x0f12, 0x0000},
	{0x0f12, 0x0000},
	{0x002a, 0x0262},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x02a6},//PCFG
	{0x0f12, 0x0780},//WIDTH
	{0x0f12, 0x0438},//HEIGHT
	{0x002a, 0x02bc},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0x0f12, 0x0000},
	{0x0f12, 0x029a},
	{0x0f12, 0x029a},
	{0x002a, 0x022c},
	{0x0f12, 0x0001},
		//for AF
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x028c},
	{0x0f12, 0x0005},
	
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x0266},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0268},
	{0x0f12, 0x0001},
	{0x002a, 0x026e},
	{0x0f12, 0x0000},
	{0x002a, 0x026a},
	{0x0f12, 0x0001},
	{0x002a, 0x0270},
	{0x0f12, 0x0001},
	{0x002a, 0x024e},
	{0x0f12, 0x0001},
	{0x002a, 0x023e},
	{0x0f12, 0x0001},
	{0x0f12, 0x0001},
	{0xffff, 0x002c},//delay
    */
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorRegVal(0xfcfc, 0xd000),
	SensorRegVal(0x0010, 0x0001),
	SensorRegVal(0x1030, 0x0000),
	SensorRegVal(0x0014, 0x0001),
	SensorRegVal(0xffff, 0x0010),//delay 10ms	
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorEnd
};

/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x077f},
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x06d0},
	{0x002a, 0x04be},
	{0x0f12, 0x03d0},
	{0x002a, 0x04c2},
	{0x0f12, 0x0520},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x0620},
	{0x002a, 0x04be},
	{0x0f12, 0x03d0},
	{0x002a, 0x04c2},
	{0x0f12, 0x0580},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	//Office
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x06c0},
	{0x002a, 0x04be},
	{0x0f12, 0x0440},
	{0x002a, 0x04c2},
	{0x0f12, 0x07a0},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd

};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	//Home
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x04e6},
	{0x0f12, 0x0777},
	{0x002a, 0x04ba},
	{0x0f12, 0x0540},
	{0x002a, 0x04be},
	{0x0f12, 0x03d0},
	{0x002a, 0x04c2},
	{0x0f12, 0x08f0},
	{0x002a, 0x04c6},
	{0x0f12, 0x0001},

	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//	Brightness 0

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//	Brightness +2

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//	Brightness +3

	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0000},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0001},*/
	 SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0004},
*/
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
/*	//Negative
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0003},
*/
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	// Bluish
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0007},
*/
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//	Greenish
/*	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023c},
	{0x0f12, 0x0009},
*/
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0080},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x00a0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x00c0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x00e0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0100},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0120},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0140},

	SensorEnd
};
/*
static	struct rk_sensor_reg sensor_Exposure7[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0160},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure8[]=
{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	{0x002a, 0x023a},
	{0x0f12, 0x0180},

	SensorEnd
};
*/


static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,    //sensor_Exposure7,sensor_Exposure8,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
//	{0x3a00, 0x78},
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
{0x002A, 0x0638},
{0x0F12, 0x0001},
{0x0F12, 0x0000},
{0x0F12, 0x1478},
{0x0F12, 0x0000},
{0x0F12, 0x1A0A},
{0x0F12, 0x0000},
{0x0F12, 0x6810},
{0x0F12, 0x0000},
{0x0F12, 0x6810},
{0x0F12, 0x0000},
{0x0F12, 0xD020},
{0x0F12, 0x0000},
{0x0F12, 0x0428},
{0x0F12, 0x0001},
{0x0F12, 0x1A80},
{0x0F12, 0x0006},
{0x0F12, 0x1A80},
{0x0F12, 0x0006},
{0x0F12, 0x1A80},
{0x0F12, 0x0006},
	//15fps ~ 3.75fps night mode for 60/50Hz light environment, 24Mhz clock input,24Mzh pclk
/*	{0x3034 ,0x1a},
	{0x3035 ,0x21},
	{0x3036 ,0x46},
	{0x3037 ,0x13},
	{0x3038 ,0x00},
	{0x3039 ,0x00},
	{0x3a00 ,0x7c},
	{0x3a08 ,0x01},
	{0x3a09 ,0x27},
	{0x3a0a ,0x00},
	{0x3a0b ,0xf6},
	{0x3a0d ,0x04},
	{0x3a0e ,0x04},
	{0x3a02 ,0x0b},
	{0x3a03 ,0x88},
	{0x3a14 ,0x0b},
	{0x3a15 ,0x88},*/
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_JPEG} 
};
/*static struct soc_camera_ops sensor_ops;*/


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
static int sensor_parameter_record(struct i2c_client *client)
{
#if 0
	u8 ret_l,ret_m,ret_h;
	int tp_l,tp_m,tp_h;
	struct generic_sensor*sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);
	sensor_read(client,0x3a00, &ret_l);
	sensor_write(client,0x3a00, ret_l&0xfb);

	sensor_write(client,0x3503,0x07);	//stop AE/AG
	sensor_read(client,0x3406, &ret_l);
	sensor_write(client,0x3406, ret_l|0x01);

	sensor_read(client,0x3500,&ret_h);
	sensor_read(client,0x3501, &ret_m);
	sensor_read(client,0x3502, &ret_l);
	tp_l = ret_l;
	tp_m = ret_m;
	tp_h = ret_h;
	spsensor->parameter.preview_exposure = ((tp_h<<12) & 0xF000) | ((tp_m<<4) & 0x0FF0) | ((tp_l>>4) & 0x0F);
	
	//Read back AGC Gain for preview
	sensor_read(client,0x350b, &ret_l);
	spsensor->parameter.preview_gain = ret_l;
	
	SENSOR_DG(" %s Read 0x350b=0x%02x  PreviewExposure:%d 0x3500=0x%02x  0x3501=0x%02x 0x3502=0x%02x \n",
	 SENSOR_NAME_STRING(), tp_l,spsensor->parameter.preview_exposure,tp_h, tp_m, tp_l);
#endif
	return 0;
}
#define OV5640_FULL_PERIOD_PIXEL_NUMS_HTS		  (2844) 
#define OV5640_FULL_PERIOD_LINE_NUMS_VTS		  (1968) 
#define OV5640_PV_PERIOD_PIXEL_NUMS_HTS 		  (1896) 
#define OV5640_PV_PERIOD_LINE_NUMS_VTS			  (984) 
static int sensor_ae_transfer(struct i2c_client *client)
{
#if 0
	u8	ExposureLow;
	u8	ExposureMid;
	u8	ExposureHigh;
	u16 ulCapture_Exposure;
	u16 Preview_Maxlines;
	u8	Gain;
	u16 OV5640_g_iExtra_ExpLines;
	struct generic_sensor*sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);
	//Preview_Maxlines = sensor->parameter.preview_line_width;
	Preview_Maxlines = spsensor->parameter.preview_maxlines;
	Gain = spsensor->parameter.preview_gain;
	
	
	ulCapture_Exposure = (spsensor->parameter.preview_exposure*OV5640_PV_PERIOD_PIXEL_NUMS_HTS)/OV5640_FULL_PERIOD_PIXEL_NUMS_HTS;
					
	SENSOR_DG("cap shutter calutaed = %d, 0x%x\n", ulCapture_Exposure,ulCapture_Exposure);
	
	// write the gain and exposure to 0x350* registers	
	sensor_write(client,0x350b, Gain);	

	if (ulCapture_Exposure <= 1940) {
		OV5640_g_iExtra_ExpLines = 0;
	}else {
		OV5640_g_iExtra_ExpLines = ulCapture_Exposure - 1940;
	}
	SENSOR_DG("Set Extra-line = %d, iExp = %d \n", OV5640_g_iExtra_ExpLines, ulCapture_Exposure);

	ExposureLow = (ulCapture_Exposure<<4)&0xff;
	ExposureMid = (ulCapture_Exposure>>4)&0xff;
	ExposureHigh = (ulCapture_Exposure>>12);
	
	sensor_write(client,0x350c, (OV5640_g_iExtra_ExpLines&0xff00)>>8);
	sensor_write(client,0x350d, OV5640_g_iExtra_ExpLines&0xff);
	sensor_write(client,0x3502, ExposureLow);
	sensor_write(client,0x3501, ExposureMid);
	sensor_write(client,0x3500, ExposureHigh);

	//SENSOR_DG(" %s Write 0x350b=0x%02x 0x350c=0x%2x  0x350d=0x%2x 0x3502=0x%02x 0x3501=0x%02x 0x3500=0x%02x\n",SENSOR_NAME_STRING(), Gain, ExposureLow, ExposureMid, ExposureHigh);
	mdelay(100);
#endif
	return 0;
}
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor  
*/
static int sensor_activate_cb(struct i2c_client *client)
{
	SENSOR_DG("%s",__FUNCTION__);
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
    struct generic_sensor *sensor = to_generic_sensor(client);
    
	SENSOR_DG("%s",__FUNCTION__);
#if 0	
	if (sensor->info_priv.funmodule_state & SENSOR_INIT_IS_OK) {
	    sensor_write(client, 0x3017, 0x00);  // FREX,VSYNC,HREF,PCLK,D9-D6
        sensor_write(client, 0x3018, 0x03);  // D5-D0
        sensor_write(client,0x3019,0x00);    // STROBE,SDA
    }
#endif    
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	//struct generic_sensor*sensor = to_generic_sensor(client);	
	if (capture) {
		sensor_parameter_record(client);
	}

	return 0;
}
static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	
	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	u16 rdval = 0xffff;
	int ret;
	ret=sensor_write_reg2val2(client, 0x002c, 0x7000);
	ret=sensor_write_reg2val2(client, 0x002e, 0x01A4);
	ret=sensor_read_reg2val2(client,  0x0f12, &rdval);
	return rdval;
}

/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	//struct generic_sensor*sensor = to_generic_sensor(client);

	if (capture) {
		sensor_ae_transfer(client);
	}
	msleep(400);

	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
		
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");
		
	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}

	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	
	SENSOR_DG("mirror: %d",mirror);

	return 0;    
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback	
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);

	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	SENSOR_DG("flip: %d",flip);

	return 0;    
}
/*
* the function is v4l2 control V4L2_CID_VFLIP callback	
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);

	return 0;
}
/*
* the functions are focus callbacks
*/
/*
for 5640 focus
*/
struct af_cmdinfo
{
	char cmd_tag;
	char cmd_para[4];
	char validate_bit;
};
static int sensor_af_cmdset(struct i2c_client *client, int cmd_main, struct af_cmdinfo *cmdinfo)
{
#if 0
	int i;
	char read_tag=0xff,cnt;

	if (cmdinfo) {
		for (i=0; i<4; i++) {
			if (cmdinfo->validate_bit & (1<<i)) {
				if (sensor_write(client, CMD_PARA0_Reg+i, cmdinfo->cmd_para[i])) {
					SENSOR_TR("%s write CMD_PARA_Reg(main:0x%x para%d:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main,i,cmdinfo->cmd_para[i]);
					goto sensor_af_cmdset_err;
				}
				SENSOR_DG("%s write CMD_PARA_Reg(main:0x%x para%d:0x%x) success!\n",SENSOR_NAME_STRING(),cmd_main,i,cmdinfo->cmd_para[i]);
			}
		}
	
		if (cmdinfo->validate_bit & 0x80) {
			if (sensor_write(client, CMD_ACK_Reg, cmdinfo->cmd_tag)) {
				SENSOR_TR("%s write CMD_ACK_Reg(main:0x%x tag:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main,cmdinfo->cmd_tag);
				goto sensor_af_cmdset_err;
			}
			SENSOR_DG("%s write CMD_ACK_Reg(main:0x%x tag:0x%x) success!\n",SENSOR_NAME_STRING(),cmd_main,cmdinfo->cmd_tag);
		}
		
	} else {
		if (sensor_write(client, CMD_ACK_Reg, 0x01)) {
			SENSOR_TR("%s write CMD_ACK_Reg(main:0x%x no tag) error!\n",SENSOR_NAME_STRING(),cmd_main);
			goto sensor_af_cmdset_err;
		}
		SENSOR_DG("%s write CMD_ACK_Reg(main:0x%x no tag) success!\n",SENSOR_NAME_STRING(),cmd_main);
	}

	if (sensor_write(client, CMD_MAIN_Reg, cmd_main)) {
		SENSOR_TR("%s write CMD_MAIN_Reg(main:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main);
		goto sensor_af_cmdset_err;
	}

	cnt = 0;
	do
	{
		msleep(5);
		if (sensor_read(client,CMD_ACK_Reg,&read_tag)){
		   SENSOR_TR("%s[%d] read TAG failed\n",SENSOR_NAME_STRING(),__LINE__);
		   break;
		}
	} while((read_tag != 0x00)&& (cnt++<100));

	SENSOR_DG("%s write CMD_MAIN_Reg(main:0x%x read tag:0x%x) success!\n",SENSOR_NAME_STRING(),cmd_main,read_tag);
#endif
	return 0;
sensor_af_cmdset_err:
	return -1;
}
static int sensor_af_idlechk(struct i2c_client *client)
{

	int ret = 0;
#if 0

	char state; 
	struct af_cmdinfo cmdinfo;
	
	SENSOR_DG("%s , %d\n",__FUNCTION__,__LINE__);
	
	cmdinfo.cmd_tag = 0x01;
	cmdinfo.validate_bit = 0x80;
	ret = sensor_af_cmdset(client, ReturnIdle_Cmd, &cmdinfo);
	if(0 != ret) {
		SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
		ret = -1;
		goto sensor_af_idlechk_end;
	}
	

	do{
		ret = sensor_read(client, CMD_ACK_Reg, &state);
		if (ret != 0){
		   SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
		   ret = -1;
		   goto sensor_af_idlechk_end;
		}
	}while(0x00 != state);
#endif
	SENSOR_DG("%s , %d\n",__FUNCTION__,__LINE__);
sensor_af_idlechk_end:
	return ret;
}
/*for 5640 focus end*/
//
static int sensor_focus_af_single_usr_cb(struct i2c_client *client);

static int sensor_focus_init_usr_cb(struct i2c_client *client)
{
    int ret = 0, cnt;
    char state;
    
	sensor_write(client, 0xFCFC , 0xD000);
	sensor_write(client, 0x0028 , 0x7000);
	//sensor_write(client, 0x002A , 0x028E);
	//sensor_write(client, 0x0F12 , 0x0000);
	sensor_write(client, 0x002A , 0x028C);
	sensor_write(client, 0x0F12 , 0x0003);
   
  SENSOR_DG("%s , %d\n",__FUNCTION__,__LINE__);
    
#if 0

    msleep(1);
    ret = sensor_write_array(client, sensor_af_firmware);
    if (ret != 0) {
    	SENSOR_TR("%s Download firmware failed\n",SENSOR_NAME_STRING());
    	ret = -1;
    	goto sensor_af_init_end;
    }

    cnt = 0;
    do
    {
    	msleep(1);
    	if (cnt++ > 500)
    		break;
    	ret = sensor_read(client, STA_FOCUS_Reg, &state);
    	if (ret != 0){
    	   SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
    	   ret = -1;
    	   goto sensor_af_init_end;
    	}
    } while (state != S_IDLE);

    if (state != S_IDLE) {
    	SENSOR_TR("%s focus state(0x%x) is error!\n",SENSOR_NAME_STRING(),state);
    	ret = -1;
    	goto sensor_af_init_end;
    }
#endif
sensor_af_init_end:
    SENSOR_DG("%s %s ret:0x%x \n",SENSOR_NAME_STRING(),__FUNCTION__,ret);
    return ret;
}
static struct rk_sensor_reg sensor_af_single_trig_regs[] = {
{0xFCFC,0xD000},
{0x0028,0x7000},
{0x002A, 0x163E},
{0x0F12, 0x00C0},
{0x0F12, 0x0080},
{0x002A, 0x15E8},
{0x0F12, 0x0010}, //af_pos_usTableLastInd
{0x0F12, 0x0018}, //af_pos_usTable
{0x0F12, 0x0020}, //af_pos_usTable
{0x0F12, 0x0028}, //af_pos_usTable
{0x0F12, 0x0030}, //af_pos_usTable
{0x0F12, 0x0038}, //af_pos_usTable
{0x0F12, 0x0040}, //af_pos_usTable
{0x0F12, 0x0048}, //af_pos_usTable
{0x0F12, 0x0050}, //af_pos_usTable
{0x0F12, 0x0058}, //af_pos_usTable
{0x0F12, 0x0060}, //af_pos_usTable
{0x0F12, 0x0068}, //af_pos_usTable
{0x0F12, 0x0070}, //af_pos_usTable
{0x0F12, 0x0078}, //af_pos_usTable
{0x0F12, 0x0080}, //af_pos_usTable
{0x0F12, 0x0088}, //af_pos_usTable
{0x0F12, 0x0090}, //af_pos_usTable
{0x0F12, 0x0098}, //af_pos_usTable
{0xFCFC,0xD000},
{0x0028,0x7000},
{0x002A,0x028C},
{0x0F12,0x0005},
SensorEnd
};

static int sensor_focus_af_single_usr_cb(struct i2c_client *client) 
{

  
  
	int ret = 0;
	char cnt;
	u16 state;
	struct af_cmdinfo cmdinfo;
	//char s_zone[5],i;
	cmdinfo.cmd_tag = 0x01;
	cmdinfo.validate_bit = 0x80;


  sensor_write(client,0xFCFC, 0xD000),
  sensor_write(client,0x0028, 0x7000),
  sensor_write(client,0x002A, 0x028E),
  sensor_write(client,0x0F12, 0x0000),
  sensor_write(client,0x002A, 0x028C),
  sensor_write(client,0x0F12, 0x0005),
  
  //sensor_write(client,0xffff, 150),	// delay 150ms
	
	cnt = 0;
	state=0x0000;
	do
	{
		if (cnt != 0) {
			msleep(1);
		}
		cnt++;
		sensor_write(client,0x002E, 0x2EEE),
		ret = sensor_read(client, 0x0f12, &state);

	}while((state != 0x0002) && (cnt<100));

	if (state != 0x0002) {
		SENSOR_TR("%s[%d] focus state(0x%x) is error!\n",SENSOR_NAME_STRING(),__LINE__,state);
		ret = -1;
		goto sensor_af_single_end;
	} else {
		SENSOR_DG("%s[%d] single focus mode set success!\n",SENSOR_NAME_STRING(),__LINE__);    
	}
sensor_af_single_end:
	return ret;
}
static struct rk_sensor_reg sensor_af_macro_regs[] = {
{0xFCFC,0xD000},
{0x0028,0x7000},
{0x002A,0x028E}, // write [7000 028E, REG_TC_AF_AfCmdParam]
{0x0F12,0x00D0}, // write lens position from 0000 to 00FF. 0000 means infinity and 00FF means macro.
{0x002A,0x028C},
{0x0F12,0x0004}, // write [7000 028C, REG_TC_AF_AfCmd] = 0004 , manual AF.

{0xffff,150},	// delay 150ms

{0x002A,0x1648},
{0x0F12,0x9002},
SensorEnd

};

static int sensor_focus_af_near_usr_cb(struct i2c_client *client)
{
	int ret;
	SENSOR_TR("sensor_s_macro_af\n");
	ret = generic_sensor_write_array(client,sensor_af_macro_regs);
	if(ret < 0)
	  SENSOR_TR("sensor_af_macro_regs error\n");

	return 0;
}
static struct rk_sensor_reg sensor_af_infinity_regs[] = {
	{0xFCFC,0xD000},
	{0x0028,0x7000},
	{0x002A,0x028E}, // write [7000 028E, REG_TC_AF_AfCmdParam]
	{0x0F12,0x0000}, // write lens position from 0000 to 00FF. 0000 means infinity and 00FF means macro.
	{0x002A,0x028C},
	{0x0F12,0x0004}, // write [7000 028C, REG_TC_AF_AfCmd] = 0004 , manual AF.
	
	{0xffff,150},	// delay 150ms
	
	{0x002A,0x1648},
	{0x0F12,0x9002},
	SensorEnd
};

static int sensor_focus_af_far_usr_cb(struct i2c_client *client)
{
	int ret;
	
	SENSOR_TR("sensor_s_macro_af\n");
	ret = generic_sensor_write_array(client,sensor_af_infinity_regs);
	if(ret < 0)
	  SENSOR_TR("sensor_af_macro_regs error\n");
	 
	return 0;
}


static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos)
{
#if 0
	struct af_cmdinfo cmdinfo;
	sensor_af_idlechk(client);
	return 0;
	cmdinfo.cmd_tag = StepFocus_Spec_Tag;
	cmdinfo.cmd_para[0] = pos;
	cmdinfo.validate_bit = 0x81;
	return sensor_af_cmdset(client, StepMode_Cmd, &cmdinfo);
#endif
	return 0;
}

static struct rk_sensor_reg sensor_af_continues_regs[] = {
	{0xFCFC,0xD000},
	{0x0028,0x7000},
	{0x002A,0x028E}, // write [7000 028E, REG_TC_AF_AfCmdParam]
	{0x0F12,0x0000}, // write lens position from 0000 to 00FF. 0000 means infinity and 00FF means macro.
	{0x002A,0x028C},
	{0x0F12,0x0006},
	SensorEnd
};

static int sensor_focus_af_const_usr_cb(struct i2c_client *client)
{
	int ret = 0;
#if 0
	struct af_cmdinfo cmdinfo;
	cmdinfo.cmd_tag = 0x01;
	cmdinfo.cmd_para[0] = 0x00;
	cmdinfo.validate_bit = 0x81;
	sensor_af_idlechk(client);
	if (sensor_af_cmdset(client, ConstFocus_Cmd, &cmdinfo)) {
		SENSOR_TR("%s[%d] const focus mode set error!\n",SENSOR_NAME_STRING(),__LINE__);
		ret = -1;
		goto sensor_af_const_end;
	} else {
		SENSOR_DG("%s[%d] const focus mode set success!\n",SENSOR_NAME_STRING(),__LINE__);	  
	}
#endif

SENSOR_TR("sensor_focus_af_const_usr_cb\n");
ret = generic_sensor_write_array(client,sensor_af_continues_regs);
if(ret < 0)
  SENSOR_TR("sensor_focus_af_const_usr_cb error\n");

sensor_af_const_end:
	return ret;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
    return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client)
{

	int ret = 0; 
#if 0

	sensor_af_idlechk(client);
	if (sensor_af_cmdset(client, PauseFocus_Cmd, NULL)) {
		SENSOR_TR("%s pause focus mode set error!\n",SENSOR_NAME_STRING());
		ret = -1;
		goto sensor_af_pause_end;
	}
#endif
sensor_af_pause_end:
	return ret;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{

	int ret = 0;
	//struct af_cmdinfo cmdinfo;
	//int zone_tm_pos[4];
	int zone_center_pos[2];
	//struct generic_sensor*sensor = to_generic_sensor(client);    
	
	if (zone_tm_pos) {
		zone_tm_pos[0] += 1000;
		zone_tm_pos[1] += 1000;
		zone_tm_pos[2]+= 1000;
		zone_tm_pos[3] += 1000;
		zone_center_pos[0] = ((zone_tm_pos[0] + zone_tm_pos[2])>>1)*SENSOR_PREVIEW_W/2000;
		zone_center_pos[1] = ((zone_tm_pos[1] + zone_tm_pos[3])>>1)*SENSOR_PREVIEW_H/2000;
	} else {
#if CONFIG_SENSOR_FocusCenterInCapture
		zone_center_pos[0] = 32;
		zone_center_pos[1] = 24;
#else
		zone_center_pos[0] = -1;
		zone_center_pos[1] = -1;
#endif
	}
	
		SENSOR_TR("[S5K4EC][AF][Got Pos]sensor_focus_af_zoneupdate_usr_cb x1=%d, y1=%d\n",zone_tm_pos[0],zone_tm_pos[1]);
		SENSOR_TR("[S5K4EC][AF][Got Pos]sensor_focus_af_zoneupdate_usr_cb x2=%d, y2=%d\n",zone_tm_pos[2],zone_tm_pos[3]);
		SENSOR_TR("[S5K4EC][AF][Got Pos]sensor_focus_af_zoneupdate_usr_cb x2=%d, y2=%d\n",zone_center_pos[0],zone_center_pos[1]);
	

sensor_af_zone_end:
	return ret;
}

/*
face defect call back
*/
static int	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	struct rk_sensor_sequence *sensor_series;
	int i,num_series;
	
	if(spsensor && spsensor->common_sensor.info_priv.sensor_series)
	{
		sensor_series = spsensor->common_sensor.info_priv.sensor_series;
		num_series = spsensor->common_sensor.info_priv.num_series;
		for(i=0; i<num_series; i++,sensor_series++)
		{
			if ((sensor_series->gSeq_info.w==2592) && (sensor_series->gSeq_info.h==1944))
			{
				sensor_series->gSeq_info.w = 2560;
				sensor_series->gSeq_info.h = 1920;
			}
		}
	}
	
	return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();

 


