/*******************************************************************************
 * (C) Copyright IBM Corporation 2017
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Algorithms & Machines team
 *******************************************************************************/ 

class JSONmanager;

#ifndef JSONmanager_H_
#define JSONmanager_H_

#define JSON_END_THREAD 0
#define JSON_START_ARRAY 1
#define JSON_END_ARRAY 2
#define JSON_START_OBJECT 3
#define JSON_END_OBJECT 4
#define JSON_ULL 5
#define JSON_STRING 6
#define JSON_DOUBLE 7

#define JSON_MAX_STRING_LENGTH 128

#include "WKLDchar.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

class JSONmanager{
    rapidjson::StringBuffer JSONbuffer;
    std::unique_ptr<rapidjson::PrettyWriter<rapidjson::StringBuffer>> JSONwriter;
    FILE* PISAfp;
    FILE* AddJSONfp;
    unsigned long long options;
    vector<WKLDchar>*  WKLDcharForThreads;
    int procId, masterId;
    
public:
    JSONmanager(int procId, unsigned long long options, vector<WKLDchar>*  WKLDcharForThreads);
    void openMaster(char* PISAFileName, char* AddJSONData, char* AppName, char* TestName);
    void openSlave(int masterId); // FIXME: parameters masterId
    void dump(int sourceId);
    void close();

    /*
        The following functions are emulating the JSONwriter
        but either write directly to the JSONwriter (if master)
        or forward the data to the master (if slave)
    */

    void StartArray();
    void EndArray();
    void StartObject();
    void EndObject();
    void Uint64(unsigned long long);
    void String(const char*);
    void Double(double);
    
    private:
        void dumpFrom(int sourceId);
        void dump_JSON_header(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter,char* AppName,char* TestName);
        void dump_JSON_footer(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter, rapidjson::StringBuffer* JSONbuffer, FILE* PISAfp);
        void dump_additional_data(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter,
                                  bool& hasApp, bool& hasSenario, bool& hasExecmd, bool& hasTime);
        void dump_additional_object(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter, rapidjson::Value* obj);
};

#endif
