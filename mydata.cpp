#include "mydata.h"

QString MyData::codSn   = "47";
QString MyData::nhSn    = "68";
QString MyData::nhNaSn  = "768";
QString MyData::ldoSn   = "01";
QString MyData::condSn  = "09";
QString MyData::phSn    = "43";
QString MyData::orpSn   = "44";
QString MyData::chlaSn  = "36";
QString MyData::bgaSn   = "62";
QString MyData::turbSn  = "29";
QString MyData::oilSn   = "BX-Oil-1N";
QString MyData::oiwSn   = "63";
QString MyData::sondeSn = "38";
QString MyData::specSn  = "";
QString MyData::chloSn  = "";
QString MyData::nitrSn  = "";

QVariantList MyData::timeList;
QVariantList MyData::probeList;
QVariantList MyData::paramList;
QVariantList MyData::dataList;
QVariantList MyData::unitList;

float MyData::currentK      = 1;
float MyData::currentB      = 0;
float MyData::currentPeriod = 0;

float MyData::chlaChla  = 0;
float MyData::chlaTemp  = 0;

float MyData::condTemp  = 0;
float MyData::condCond  = 0;

float MyData::sondeLdo  = 0;
float MyData::sondeTurb = 0;
float MyData::sondeCond = 0;
float MyData::sondePH   = 0;
float MyData::sondeChla = 0;
float MyData::sondeBga  = 0;
float MyData::sondeOrp  = 0;
float MyData::sondeTemp = 0;

float MyData::turbTemp  = 0;
float MyData::turbTurb  = 0;

float MyData::ldoTemp   = 0;
float MyData::ldoLdo    = 0;
float MyData::ldoSaturation = 0;

float MyData::oilOil    = 0;
float MyData::oilTemp   = 0;

float MyData::codCod    = 0;
float MyData::codToc    = 0;
float MyData::codTurb   = 0;
float MyData::codTemp   = 0;

float MyData::bgaBga    = 0;
float MyData::bgaTemp   = 0;

float MyData::nhOrp		= 0;
float MyData::nhPH      = 0;
float MyData::nhNh4Plus = 0;
float MyData::nhKPlus   = 0;
float MyData::nhNh4_N   = 0;
float MyData::nhTemp    = 0;

float MyData::phPH      = 0;
float MyData::phOrp     = 0;
float MyData::phTemp    = 0;

float MyData::orpOrp    = 0;
float MyData::orpPH     = 0;
float MyData::orpTemp   = 0;


QString MyData::testSn1     = "0";
bool MyData::clickedEvent   = false;
bool MyData::isRun          = false;

bool MyData::isSondeCal = false;
QString MyData::calParam = "ldo";

QDateTime MyData::sqlTime   = QDateTime::currentDateTime();
int MyData::timePeroid      = 1;
int MyData::firstSave       = 0;

QString MyData::oilType = "oil";
QString MyData::nhType  = "nhn";

int MyData::sqlServerStatus = 0;

bool MyData::idRepeat = false;

MyData::MyData()
{

}
