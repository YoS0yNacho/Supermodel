
//scudplus.zip


#define gCredits 0x1000f3
#define gCreditMode 0x100114
#define gLink      0x10011D
#define gCarNumber 0x10011C
//----------------------
#define gMainState 0x104006
#define gSubState  0x104005

#define msMainMenu 11

#define gMainTimer 0x104010
#define gSubTimer  0x104008
#define gCourseLaps 0x104015
#define gCPUCounter 0x104018
#define gCarCount   0x10401C
#define gLocalPlayerCar 0x104F47
#define p1TMission 0x104f45 
#define gCourse  0x104F47
//--------------------------------
#define gRealPlayers 0x10F024
#define gRPArrows 0x10F030
#define ENABLEARROWS 0x3
//--------------------

#define bCanMove 0x00      //4 byte
#define bCarType 0x06      //byte
#define bCarOwner 0x07     //byte
#define bVelocimeter 0x18  //word
#define bXPos 0x24       //float
#define bYPos 0x1C       //float
#define bZPos 0x20       //float
#define bPitch 0x2A      //float
#define bYaw 0x48        //float
#define bSpeed 0x34      //float
#define bCarNumber 0x98  // byte 
#define bSteer 0xC0      //float or 4bytes not sure
#define bGasPedal 0xE4   //float ??
#define bDisableAI 0x1e0 // byte

//----------------------------

#define cBus 0x0
#define cCat 0x1
#define cTank 0x2
#define cRocket 0x3
#define c911 0x4
#define cViper 0x5
#define cF40 0x6
#define cMcF1 0x7


//patches
#define SecretCarsAddr 0x2BB90
#define SecretCarsON = 0x60000000;
#define SecretCarsOFF = 0x4082001C;

#define SelCourseFixAddr 0x2ABE4
#define SelCourseFixON = 0x60000000;
#define SelCourseFixOFF = 0x409A000C;

#define DisableRetireAddr 0x0002883C
#define DisableRetireAddr2 0x00028714
#define DisableRetireAddr3 0x00028744
#define DisableRetireAddr4 0x00028748