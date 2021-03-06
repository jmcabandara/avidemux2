// automatically generated by admSerialization.py, do not edit!
#include "ADM_default.h"
#include "ADM_paramList.h"
#include "ADM_coreJson.h"
#include "asharp.h"
bool  asharp_jserialize(const char *file, const asharp *key){
admJson json;
json.addFloat("t",key->t);
json.addFloat("d",key->d);
json.addFloat("b",key->b);
json.addBool("bf",key->bf);
json.addBool("d_enabled",key->d_enabled);
json.addBool("b_enabled",key->b_enabled);
return json.dumpToFile(file);
};
bool  asharp_jdeserialize(const char *file, const ADM_paramList *tmpl,asharp *key){
admJsonToCouple json;
CONFcouple *c=json.readFromFile(file);
if(!c) {ADM_error("Cannot read json file");return false;}
bool r= ADM_paramLoadPartial(c,tmpl,key);
delete c;
return r;
};
